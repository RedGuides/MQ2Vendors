// MQ2Vendors.cpp
// - plugin to monitor a vendors item list and notify you if they have items you want to buy.

/**
* TODO:
* Add more information to the TLO(Vendor). Need to have it list the items
* your currently searching on available on the current merchant, and their
* respective locations slot locations so you can:
*
*   /notify MerchantWnd MW_ItemList listselect ${slotnum}
*   /delay 5
*   /notify MerchantWnd MW_ItemList leftmouse  ${slotnum}
*   /delay 1s
*
*/

#include "mq/Plugin.h"
#include <chrono>

PreSetup("MQ2Vendors");
PLUGIN_VERSION(1.02);


using namespace mq::datatypes;
using namespace std::chrono;

constexpr int MAX_VENDOR_SLOTS = 80;

std::vector<std::string> SearchList;
char MDBINIFile[MAX_PATH] = {};
int iSaveVendors = 0;
bool DEBUGGING = false;

static bool s_windowOpen = false;
static bool s_waitForItems = false;
static bool s_scanMerchantWindow = true;
static steady_clock::time_point s_scanMerchantWindowTime = {};
static seconds s_scanMerchantDelay{ 2 };


struct ItemDataRec
{
	std::string Name;
	int Plat;
	int Gold;
	int Silver;
	int Copper;
	int Quantity;
	int Slot;
};

std::vector<ItemDataRec> ItemData;

class MQ2VendorsType : public MQ2Type
{
public:
	enum class VendorsMembers
	{
		Version = 1,
		HasItems,
		Items,
		Count,
	};

	MQ2VendorsType() : MQ2Type("Vendor")
	{
		ScopedTypeMember(VendorsMembers, Version);
		ScopedTypeMember(VendorsMembers, HasItems);
		ScopedTypeMember(VendorsMembers, Items);
		ScopedTypeMember(VendorsMembers, Count);
	}

	virtual bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
	{
		MQTypeMember* pMember = MQ2VendorsType::FindMember(Member);

		if (!pMember)
			return false;

		if (!pMerchantWnd)
			return false;

		CXWnd* pPurchasePage = pMerchantWnd->GetChildItem("MW_PurchasePage");
		if (!pPurchasePage)
			return false;

		CListWnd* pItemList = static_cast<CListWnd*>(pPurchasePage->GetChildItem("MW_ItemList"));
		if (!pItemList)
			return false;

		switch (static_cast<VendorsMembers>(pMember->ID))
		{
		case VendorsMembers::Version:
			Dest.Float = MQ2Version;
			Dest.Type = pFloatType;
			return true;

		case VendorsMembers::HasItems:
			Dest.Set(false);
			Dest.Type = pBoolType;

			for (int i = 0; i < pItemList->GetItemCount(); i++)
			{
				CXStr str = pItemList->GetItemText(i, 1);
				if (!str.empty())
				{
					for (const std::string& item : SearchList)
					{
						if (item.compare(str) == 0)
						{
							Dest.Set(true);
							break;
						}
					}
				}
			}
			return true;

		case VendorsMembers::Items:
			if (Index[0])
			{
				int val = GetIntFromString(Index, 0) - 1;
				if (val >= 0 && val < static_cast<int>(ItemData.size()))
				{
					// could add other members to this (another type), that would let you show any member (quantity, gold, etc.)
					// this would let you do like:  ${Vendor.Items[1].Slot}, ${Vendor.Items[1].Quantity}, etc.
					Dest.Type = pStringType;
					strcpy_s(DataTypeTemp, ItemData[val].Name.c_str());
					Dest.Ptr = DataTypeTemp;
					return true;
				}
			}
			return false;

		case VendorsMembers::Count:
			Dest.Int = static_cast<int>(ItemData.size());
			Dest.Type = pIntType;
			return true;
		}
		return false;
	}
};

MQ2VendorsType* pVendorsType = nullptr;

bool IsInGame()
{
	return gGameState == GAMESTATE_INGAME && pLocalPC && pLocalPlayer;
}

PLUGIN_API void VendorDebug(PlayerClient*, const char* Cmd)
{
	char zParm[MAX_STRING] = {};
	GetArg(zParm, Cmd, 1);

	if (zParm[0] == 0)
		DEBUGGING = !DEBUGGING;
	else if (!_strnicmp(zParm, "on", 2))
		DEBUGGING = true;
	else if (!_strnicmp(zParm, "off", 2))
		DEBUGGING = false;
	else
		DEBUGGING = !DEBUGGING;

	WriteChatf("\arMQ2Vendors\ax::\amDEBUGGING is now %s\ax.", DEBUGGING ? "\aoON" : "\agOFF");
}

bool WindowOpen(const char* WindowName)
{
	CXWnd* pWnd = FindMQ2Window(WindowName);
	return pWnd && pWnd->IsVisible();
}

void InitSearchList()//Exceeds Stack
{
	SearchList.clear();
	ItemData.clear();

	std::vector<std::string> keys = GetPrivateProfileKeys<MAX_STRING * 10>("ItemsList", INIFileName);

	for (const std::string& key : keys)
	{
		std::string value = GetPrivateProfileString("ItemsList", key, std::string(), INIFileName);
		if (DEBUGGING)
			WriteChatf("MQ2Vendors: ItemsList %s=%s", key.c_str(), value.c_str());
		if (!value.empty())
			SearchList.push_back(std::move(value));
	}
}

void ClearSearchList()
{
	SearchList.clear();
	ItemData.clear();
}

void VendorUsage()
{
	SyntaxError("Usage: /vendor \au<command>\ax \ap<string>\ax");
	WriteChatf("  Valid commands are:");
	WriteChatf("     \ausavevendors\ax - enables saving vendor base items");
	WriteChatf("     \aunosavevendors\ax - disables saving vendor base items");
	WriteChatf("     \aulist\ax - lists the current items being watched for");
	WriteChatf("     \aufound\ax - lists items found on the current merchant from your list");
	WriteChatf("     \auadd\ax - adds \ap<string>\ax to the search list");
	WriteChatf("     \auremove\ax - removes \ap<string>\ax to the search list");
	WriteChatf("     \ap<string>\ax can be an item link, MQ2Vendors will remove the extra link data before saving the item name.");
}

void VendorCmd(PlayerClient*, const char* szLine)
{
	char nItem[7] = {};
	char szArg[MAX_STRING] = {};

	if (szLine[0] == 0)
	{
		VendorUsage();
		return;
	}

	GetArg(szArg, szLine, 1);
	const char* szRest = GetNextArg(szLine);

	if (DEBUGGING)
	{
		WriteChatf("MQ2Vendors:  szArg=%s", szArg);
		WriteChatf("MQ2Vendors: szRest=%s", szRest);
	}

	if (ci_equals(szArg, "savevendors"))
	{
		WriteChatf("MQ2Vendors: Now Saving Vendor Item List");
		iSaveVendors = 1;
		sprintf_s(nItem, "%d", iSaveVendors);
		WritePrivateProfileString("Config", "SaveVendors", nItem, INIFileName);
		return;
	}

	if (ci_equals(szArg, "nosavevendors"))
	{
		WriteChatf("MQ2Vendors: Not Saving Vendor Item List");
		iSaveVendors = 0;
		sprintf_s(nItem, "%d", iSaveVendors);
		WritePrivateProfileString("Config", "SaveVendors", nItem, INIFileName);
		return;
	}

	if (ci_equals(szArg, "list"))
	{
		WriteChatf("MQ2Vendors: I am watching for %d items.", SearchList.size());

		int index = 1;
		for (const std::string& item : SearchList)
		{
			WriteChatf("MQ2Vendors: %d: \ap%s\ax", index, item.c_str());
			index++;
		}
		return;
	}

	if (!strcmp(szArg, "found"))
	{
		if (!WindowOpen("MerchantWnd"))
		{
			WriteChatf("MQ2Vendors: Merchant window is not open.");
		}
		else
		{
			if (ItemData.empty())
			{
				WriteChatf("MQ2Vendors: Merchant doesn't have any items on your list.");
			}
			else
			{
				WriteChatf("MQ2Vendors: Merchant has %zu %s on your list:", ItemData.size(), ItemData.size() == 1 ? "item" : "items");

				for (const auto& item : ItemData)
				{
					WriteChatf("MQ2Vendors: >>> \ap%s\ax X \ar%s\ax(\atCost\ax: %dp %dg %ds %dc) \atItemSlot\ax: \ar%i",
						item.Name.c_str(), item.Quantity, item.Plat, item.Gold, item.Silver, item.Copper, item.Slot + 1);
				}
			}
		}
		return;
	}

	if (!szRest || !szRest[0])
	{
		VendorUsage();
		return;
	}

	// Parse a single link.
	std::string_view itemName = mq::trim(std::string_view(szRest));
	TextTagInfo tagInfo = ExtractLink(itemName);

	if (tagInfo.tagCode == ETAG_ITEM)
	{
		itemName = tagInfo.text;
	}

	if (itemName.empty())
		return;
	
	if (ci_equals(szArg, "add"))
	{
		auto iter = std::find_if(SearchList.begin(), SearchList.end(),
			[&itemName](const std::string& item) { return ci_equals(itemName, item); });
		if (iter != SearchList.end())
		{
			WriteChatf("MQ2Vendors: \ap%s\ax is already in your list.", iter->c_str());
		}
		else
		{
			SearchList.emplace_back(itemName);
			WriteChatf("MQ2Vendors: Added \ap%.*s\ax to your list.", itemName.length(), itemName.data());
		}
	}
	else if (!strcmp(szArg, "remove"))
	{
		auto iter = std::remove_if(SearchList.begin(), SearchList.end(),
			[&](const std::string& item) { return ci_equals(itemName, item); });
		if (iter == SearchList.end())
		{
			WriteChatf("MQ2Vendors: \ap%.*s\ax was not found in your list.", itemName.length(), itemName.data());
		}
		else
		{
			SearchList.erase(iter, SearchList.end());
			WriteChatf("MQ2Vendors: Removed \ap%.*s\ax from your list.", itemName.length(), itemName.data());
		}
	}
	else
	{
		WriteChatf("MQ2Vendors: unknown command %s", szArg);
		VendorUsage();
		return;
	}

	std::sort(SearchList.begin(), SearchList.end());
	auto last = std::unique(SearchList.begin(), SearchList.end());
	SearchList.erase(last, SearchList.end());


	WritePrivateProfileSection("ItemsList", nullptr, INIFileName);

	int index = 1;
	std::string sectionName = "ItemsList";
	for (const std::string& item : SearchList)
	{
		mq::WritePrivateProfileString(sectionName, fmt::format("Item{}", index), item, INIFileName);
		++index;
	}
}

bool dataVendors(const char*, MQTypeVar& Ret)
{
	Ret.DWord = 1;
	Ret.Type = pVendorsType;
	return true;
}

PLUGIN_API void InitializePlugin()
{
	sprintf_s(MDBINIFile, "%s\\MQ2Vendors-db.ini", gPathConfig);
	iSaveVendors = GetPrivateProfileInt("Config", "SaveVendors", 0, INIFileName);
	InitSearchList();
	AddMQ2Data("Vendor", dataVendors);
	AddCommand("/vendor", VendorCmd);
	AddCommand("/vendordebug", VendorDebug);
	pVendorsType = new MQ2VendorsType;
}

PLUGIN_API void ShutdownPlugin()
{
	RemoveCommand("/vendor");
	RemoveCommand("/vendordebug");
	RemoveMQ2Data("Vendor");
	ClearSearchList();
	delete pVendorsType;
}

static CListWnd* GetMerchantListWindow()
{
	// check our items list. we wait 2 seconds or end if we have MAX_VENDOR_SLOTS items.
	CXWnd* purchasePage = pMerchantWnd->GetChildItem("MW_PurchasePage");
	if (!purchasePage)
	{
		if (DEBUGGING)
			WriteChatf("MQ2Vendors: Could not get MW_PurchasePage.");

		return nullptr;
	}

	CListWnd* cLWnd = cLWnd = static_cast<CListWnd*>(purchasePage->GetChildItem("MW_ItemList"));
	if (!cLWnd)
	{
		if (DEBUGGING)
			WriteChatf("MQ2Vendors: Could not get MW_ItemList.");

		return nullptr;
	}

	if (cLWnd->GetType() != UI_Listbox)
	{
		if (DEBUGGING)
			WriteChatf("MQ2Vendors: ItemList is not a listbox!?");

		return nullptr;
	}

	return cLWnd;
}

PLUGIN_API void OnPulse()
{
	// Check if window state has changed
	bool windowOpen;

	if (!IsInGame())
	{
		windowOpen = false;
	}
	else
	{
		windowOpen = pMerchantWnd && pMerchantWnd->IsVisible();
	}

	if (s_windowOpen != windowOpen)
	{
		s_windowOpen = windowOpen;

		if (s_windowOpen)
		{
			s_scanMerchantWindow = true;
			s_waitForItems = true;

			s_scanMerchantWindowTime = steady_clock::now();

			if (DEBUGGING)
				WriteChatf("MQ2Vendors: Merchant Window opened.");
		}
		else
		{
			ItemData.clear();

			if (DEBUGGING)
				WriteChatf("MQ2Vendors: Merchant Window closed.");
		}
	}

	if (!s_windowOpen)
		return;


	// At this point the merchant window is open.

	// Wait for items to appear or two seconds, whichever happens first.
	if (s_waitForItems)
	{
		if (CListWnd* pMerchList = GetMerchantListWindow())
		{
			int slotCount = 0;

			for (int i = 0; i < MAX_VENDOR_SLOTS; i++)
			{
				CXStr itemText = pMerchList->GetItemText(i, 1);
				if (!itemText.empty())
				{
					slotCount++;
				}
			}

			if (slotCount == MAX_VENDOR_SLOTS
				|| steady_clock::now() > s_scanMerchantWindowTime + s_scanMerchantDelay)
			{
				s_waitForItems = false;
			}
		}
		else
		{
			if (DEBUGGING)
				WriteChatf("MQ2Vendors: Could not get Merchant List Window while waiting for items.");
		}

		return;
	}

	if (s_scanMerchantWindow)
	{
		s_scanMerchantWindow = false;

		char ZoneName[MAX_STRING] = {};

		if (DEBUGGING)
		{
			WriteChatf("MQ2Vendors::Merchant Window is open");
			WriteChatf("MQ2Vendors: waited %.2f seconds",
				duration_cast<milliseconds>(steady_clock::now() - s_scanMerchantWindowTime).count() / 1000.0f);
		}

		strcpy_s(ZoneName, pZoneInfo ? pZoneInfo->ShortName : "unknown");

		if (DEBUGGING)
			WriteChatf("Zone: %s", ZoneName);

		// update the merchants name(show whether they have items we want)
		CXStr merchantName = pMerchantWnd->GetFirstChildWnd()->GetWindowText();
		if (merchantName.size() > 5 && merchantName[0] == '>' && merchantName[merchantName.size() - 1] == '<')
		{
			merchantName = merchantName.substr(2, merchantName.size() - 4);
		}

		if (DEBUGGING)
			WriteChatf("Merchant Name  : %s", merchantName.c_str());

		int entryCount = 0;
		int debugCount = 0;

		CListWnd* pMerchList = GetMerchantListWindow();
		if (!pMerchList)
			return;

		// 0 - icon
		// 1 - item name
		// 2 - merchant quantity(--means always have it)
		// 3 - plat - Now the currency icon
		// 4 - gold - now the cost section
		// 5 - silver - now the level section
		// 6 - copper - no longer used.
		ItemData.clear();
		int itemCount = 0;
		bool mHeader = false;

		for (int i = 0; i < pMerchList->GetItemCount(); i++)
		{
			CXStr sItem = pMerchList->GetItemText(i, 1);
			if (!sItem.empty())
			{
				if (DEBUGGING)
					WriteChatf("GetItemText(%d,1): %s", i, sItem.c_str());

				// get the quantity for this item slot.
				CXStr Quantity = pMerchList->GetItemText(i, 2);
				CXStr cost = pMerchList->GetItemText(i, 4);

				// Convert the char array to an integer and multiply it by 1000. Had to add 1 to get that last copper to be correct.
				int iTemp = static_cast<int>(GetFloatFromString(cost, 0) * 1000) + 1;
				int cP = iTemp / 1000;
				int cG = (iTemp - (cP * 1000)) / 100;
				int cS = (iTemp - ((cP * 1000) + (cG * 100))) / 10;
				int cC = (iTemp - ((cP * 1000) + (cG * 100) + (cS * 10))) / 1;

				// if quantity == '--', item is always on the merchant.
				if (ci_equals(Quantity, "--"))
				{
					// we now have an item that the merchant has upon server reboot.
					if (iSaveVendors)
					{
						if (!mHeader)
						{
							WritePrivateProfileString(merchantName.c_str(), "Zone", ZoneName, MDBINIFile);
							mHeader = true;
						}

						mq::WritePrivateProfileString(merchantName.c_str(), fmt::format("Item{:02}", entryCount++).c_str(),
							sItem.c_str(), MDBINIFile);
					}
				}
				else if (DEBUGGING)
				{
					WriteChatf("Temp%02d = %s", debugCount++, sItem.c_str());
				}

				// does an item exist on our searchlist?
				auto iter = std::find_if(SearchList.begin(), SearchList.end(),
					[&](const std::string& item) { return ci_equals(item, sItem); });
				if (iter != SearchList.end())
				{
					itemCount++;

					ItemDataRec tItem;
					tItem.Name = sItem;
					tItem.Quantity = atoi(Quantity.c_str());
					tItem.Plat = cP;
					tItem.Gold = cG;
					tItem.Silver = cS;
					tItem.Copper = cC;
					tItem.Slot = i;

					ItemData.push_back(tItem);
					WriteChatf("MQ2Vendors: >>> \ap%s\ax X \ar%s\ax (\atCost\ax: %dp %dg %ds %dc) \atItemSlot\ax: \ar%i",
						sItem.c_str(), string_equals(Quantity, "--") ? "Infinite" : Quantity.c_str(), cP, cG, cS, cC, i + 1);
				}
			}
			else
			{
				if (DEBUGGING)
					WriteChatf("pMerch->ItemDesc[%02d] was empty", i);
			}
		}

		// update the merchants name (show whether they have items we want)
		if (itemCount)
		{
			std::string_view windowText = pMerchantWnd->GetFirstChildWnd()->GetWindowText();

			if (!windowText.empty() && windowText[0] != '>')
			{
				CXStr newWindowText(fmt::format("> {} <", windowText));

				pMerchantWnd->GetFirstChildWnd()->SetWindowText(newWindowText);
			}

			WriteChatf("MQ2Vendors: %s has \ar%d\ax item%s we are searching for!",
				merchantName.c_str(), itemCount, itemCount > 1 ? "s" : "");
		}
		else
		{
			WriteChatf("MQ2Vendors: %s has nothing we are searching for!", merchantName.c_str());
		}
	}
}
