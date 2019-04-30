// MQ2Vendors.cpp
// - plugin to monitor a vendors item list and notify you if they have items you want to buy.
//
// - Chatwiththisname 1/4/2019 - Fixed /vendor savevendors and /vendor nosavevendors to stop
//							   displaying the help list because they lacked a second parameter.
//							   - Updated getting price to actually get the price with the new
//							   vendor UI setup where the price is a floating point number instead
//							   of a section for each currency type.
//							   - Updated output messages with some color, and now shows price and
//							   slot number for each item found on initial report as well as when 
//							   "/vendor found" command is used.

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

#include "../MQ2Plugin.h"
using namespace std;
#include <list>
#include <string>
#include <vector>

PreSetup ("MQ2Vendors");

#define MQ2VENDORS_VERSION "1.02"
#define SKIP_PULSES     100
#define MAX_VENDOR_SLOTS 80
#define pMerch ((PEQMERCHWINDOW)pMerchantWnd)

list <string> SearchList;
CHAR MDBINIFile[MAX_PATH] = {0};
int iSaveVendors = 0;
bool DEBUGGING=false;

#define ISINDEX() (Index[0])
#define ISNUMBER() (IsNumber(Index))
#define GETNUMBER() (atoi(Index))
#define GETFIRST() Index

typedef struct {
    char Name[MAX_STRING];
    int Plat;
    int Gold;
    int Silver;
    int Copper;
    int Quantity;
    int Slot;
} _ItemData;

vector<_ItemData> ItemData;

class MQ2VendorsType : public MQ2Type
{
public:
    enum VendorsMembers
    {
        Version=1,
        HasItems=2,
        Items=3,
        Count=4,
    };
    MQ2VendorsType() : MQ2Type("Vendor")
    {
        TypeMember(Version);
        TypeMember(HasItems);
        TypeMember(Items);
        TypeMember(Count);
    }
    ~MQ2VendorsType()
    {
    }
    bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR &Dest)
    {
        int i;
        CHAR Copy[MAX_STRING] = {0}, DataTypeTemp[MAX_STRING] = {0};
        PMQ2TYPEMEMBER pMember=MQ2VendorsType::FindMember(Member);
        if (!pMember)
            return false;
        if (!pMerchantWnd)
            return false;
        CListWnd *cLWnd = NULL;
        CXWnd *Win=pMerchantWnd->GetChildItem("MW_PurchasePage");
        if(!Win)
            return false;
        CXWnd *Win2=Win->GetChildItem("MW_ItemList");
        if(!Win2)
            return false;
        cLWnd = (CListWnd*)((CSidlScreenWnd*)(Win->GetChildItem("MW_ItemList")));
        switch((VendorsMembers)pMember->ID)
        {
        case Version:
            Dest.Ptr= MQ2VENDORS_VERSION;
            Dest.Type=pStringType;
            return true;
        case HasItems:
            Dest.DWord = false;
            Dest.Type = pBoolType;
            for (i = 0; i < MAX_VENDOR_SLOTS; i++) {
                CXStr cxstr = *cLWnd->GetItemText (&cxstr, i, 1);
                GetCXStr (cxstr.Ptr, Copy, MAX_STRING);
                if (strlen (Copy) > 0) {
                    list<string>::iterator pList = SearchList.begin ();
                    while (pList != SearchList.end ()) {
                        if (!pList->compare(Copy))
                            Dest.DWord=true;
                        pList++;
                    }
                }
            }
            return true;
        case Items:
            if (!ISINDEX())
                return false;
            if (ISINDEX()) {
                unsigned int pIndex = (unsigned int) atoi(Index);
                if(pIndex > 0 && pIndex <= ItemData.size()) {
                    // could add other members to this (another type), that would let you show any member (quantity, gold, etc.)
                    // this would let you do like:  ${Vendor.Items[1].Slot}, ${Vendor.Items[1].Quantity}, etc.
                    Dest.Type=pStringType;
                    strcpy_s(DataTypeTemp, ItemData[pIndex-1].Name);
                    Dest.Ptr = DataTypeTemp;
                    return true;
                }
            }
            return false;
        case Count:
            Dest.Int = ItemData.size();
            Dest.Type = pIntType;
            return true;
        }
        return false;
    }

    bool ToString(MQ2VARPTR VarPtr, PCHAR Destination)
    {
        return true;
    }

    bool FromData(MQ2VARPTR &VarPtr, MQ2TYPEVAR &Source)
    {
        return false;
    }       
    bool FromString(MQ2VARPTR &VarPtr, PCHAR Source)
    {
        return false;
    }
};

class MQ2VendorsType *pVendorsType=0;

bool InGames() {
    if(!MQ2Globals::gZoning && MQ2Globals::gGameState==GAMESTATE_INGAME && GetCharInfo2() && GetCharInfo() && GetCharInfo()->pSpawn)
        return true;
    return false;
}

PLUGIN_API VOID VendorDebug(PSPAWNINFO pChar, PCHAR Cmd)
{
    char zParm[MAX_STRING];
    GetArg(zParm,Cmd,1);
    if(zParm[0]==0)
        DEBUGGING=!DEBUGGING;
    else if(!_strnicmp(zParm,"on",2))
        DEBUGGING=true;
    else if(!_strnicmp(zParm,"off",2))
        DEBUGGING=false;
    else
        DEBUGGING=!DEBUGGING;
    WriteChatf("\arMQ2Vendors\ax::\amDEBUGGING is now %s\ax.",DEBUGGING?"\aoON":"\agOFF");
}

BOOL WindowOpen(PCHAR WindowName) {
    PCSIDLWND pWnd=(PCSIDLWND)FindMQ2Window(WindowName);
    return (pWnd) ? (BOOL)pWnd->IsVisible() : false;
}

VOID InitSearchList ()
{
    char ItemsList[MAX_STRING * 10];
    SearchList.clear ();
    ItemData.clear();
    if (GetPrivateProfileString("ItemsList", NULL, "", ItemsList, MAX_STRING * 10, INIFileName)) {
        char Buffer[MAX_STRING];
        char *ilp = ItemsList;
        while (ilp[0] != NULL) {
            GetPrivateProfileString ("ItemsList", ilp, "", Buffer, MAX_STRING, INIFileName);
            if(DEBUGGING)
                WriteChatf("MQ2Vendors: ItemsList %s=%s", ilp, Buffer);
            if (Buffer[0] != NULL)
                SearchList.push_back (string (Buffer));
            ilp += strlen (ilp) + 1;
        }
    }
}

VOID ClearSearchList ()
{
    SearchList.clear();
    ItemData.clear();
}

VOID VendorUsage()
{
    SyntaxError ("Usage: /vendor \au<command>\ax \ap<string>\ax");
    WriteChatf("  Valid commands are:");
    WriteChatf("     \ausavevendors\ax - enables saving vendor base items");
    WriteChatf("     \aunosavevendors\ax - disables saving vendor base items");
    WriteChatf("     \aulist\ax - lists the current items being watched for");
    WriteChatf("     \aufound\ax - lists items found on the current merchant from your list");
    WriteChatf("     \auadd\ax - adds \ap<string>\ax to the search list");
    WriteChatf("     \auremove\ax - removes \ap<string>\ax to the search list");
    WriteChatf("       \ap<string>\ax can be an item link, MQ2Vendors will remove the extra link data before saving the item name.");
}

VOID VendorCmd (PSPAWNINFO pChar, PCHAR szLine)
{
    list<string>::iterator pList;
    CHAR nItem[7] = {0};
    CHAR szArg[MAX_STRING] = {0};
    int i;
    if (szLine[0] == 0) {
        VendorUsage();
        return;
    }
    GetArg (szArg, szLine, 1);
    PCHAR szRest = GetNextArg (szLine);
    if(DEBUGGING) {
        WriteChatf("MQ2Vendors:  szArg=%s", szArg);
        WriteChatf("MQ2Vendors: szRest=%s", szRest);
    }
    if (!strcmp (szArg, "savevendors")) {
        WriteChatf("MQ2Vendors: Now Saving Vendor Item List");
        iSaveVendors = 1;
        sprintf_s (nItem, "%d", iSaveVendors);
        WritePrivateProfileString("Config", "SaveVendors", nItem, INIFileName);
		return;
    }
    if (!strcmp (szArg, "nosavevendors")) {
        WriteChatf("MQ2Vendors: Not Saving Vendor Item List");
        iSaveVendors = 0;
        sprintf_s (nItem, "%d", iSaveVendors);
        WritePrivateProfileString("Config", "SaveVendors", nItem, INIFileName);
		return;
    }
    if (!strcmp (szArg, "list")) {
        pList = SearchList.begin ();
        WriteChatf("MQ2Vendors: I am watching for %d items.", SearchList.size());
        i = 1;                         
        while (pList != SearchList.end ()) {
            WriteChatf ("MQ2Vendors: %d: \ap%s\ax", i++, pList->c_str ());
            pList++;
        }
        return;
    }
    if (!strcmp (szArg, "found")) {
        if (!WindowOpen("MerchantWnd"))
            WriteChatf("MQ2Vendors: Merchant window is not open.");
        else {
            if(!ItemData.size())
                WriteChatf("MQ2Vendors: Merchant doesn't have any items on your list.");
            else {
                WriteChatf("MQ2Vendors: Merchant has %d %s on your list:", ItemData.size(), ItemData.size()==1?"item":"items");
                for(unsigned int x=0; x<ItemData.size(); x++)
                    WriteChatf("MQ2Vendors: >>> \ap%s\ax X \ar%s\ax(\atCost\ax: %dp %dg %ds %dc) \atItemSlot\ax: \ar%i", ItemData[x].Name, ItemData[x].Quantity, ItemData[x].Plat, ItemData[x].Gold, ItemData[x].Silver, ItemData[x].Copper, ItemData[x].Slot+1);
            }
        }
        return;
    }
    if (!szRest || !szRest[0]) {
        VendorUsage();
        return;
    }
    //check if szRest was a link (links begin with \022 and end with \022
    // be nice if there was a way to trim space from the end of the string
    CHAR Copy[MAX_STRING] = {0};
    PCHAR pCopy = szRest;
    while (*pCopy == ' ') {
        pCopy++;
    }
    if (*pCopy == 0x12) {
        //we have a link...
        strcpy_s (Copy, pCopy + 46);
    } else {
        strcpy_s (Copy, pCopy);
    }
    i = strlen(Copy) - 1;
    while (i>0 && (Copy[i] == ' ' || Copy[i] == 0x12)) {
        i--;
    }
    Copy[i+1] = 0;
    if(! Copy || !Copy[0])
        return;
    if (!strcmp (szArg, "add")) {
        bool found = false;
        pList = SearchList.begin ();
        while (pList != SearchList.end ()) {
            if(!_stricmp(pList->c_str(), Copy)) {
                found = true;
                WriteChatf ("MQ2Vendors: \ap%s\ax is already in your list.", pList->c_str ());
            }
            pList++;
        }
        if(!found) {
            SearchList.push_back (string (Copy));
            WriteChatf ("MQ2Vendors: Added \ap%s\ax to your list.", Copy);
        }
    } else if (!strcmp (szArg, "remove")) {
        bool found = false;
        pList = SearchList.begin ();
        while (pList != SearchList.end ()) {
            if(!_stricmp(pList->c_str(), Copy)) {
                strcpy_s(Copy, pList->c_str());
                found = true;
            }
            pList++;
        }
        if(found) {
            SearchList.remove (string (Copy));
            WriteChatf ("MQ2Vendors: Removed \ap%s\ax from your list.", Copy);
        }
        else
            WriteChatf ("MQ2Vendors: \ap%s\ax was not found in your list.", Copy);
    } else {
        WriteChatf ("MQ2Vendors: unknown command %s", szArg);
        VendorUsage();
        return;
    }
    SearchList.sort ();
    SearchList.unique ();
    sprintf_s (nItem, "%d", SearchList.size ());
    WritePrivateProfileSection("ItemsList", 0, INIFileName);
    i = 1;
    pList = SearchList.begin ();
    while (pList != SearchList.end ()) {
        sprintf_s (nItem, "Item%d", i++);
        WritePrivateProfileString ("ItemsList", nItem,
            pList->c_str (), INIFileName);
        pList++;
    }
}

BOOL dataVendors(PCHAR szIndex, MQ2TYPEVAR &Ret)
{
    if (szIndex != NULL)
    {
    }
    Ret.DWord = 1;
    Ret.Type = pVendorsType;
    return true;
}

PLUGIN_API VOID InitializePlugin (VOID)
{
    DebugSpewAlways ("Initializing MQ2Vendors");
    WriteChatf ("MQ2Vendors::InitializePlugin");
    sprintf_s (MDBINIFile, "%s\\%s-db.ini", gszINIPath, "MQ2Vendors");
    iSaveVendors = GetPrivateProfileInt("Config", "SaveVendors", 0, INIFileName);
    InitSearchList ();
    AddMQ2Data("Vendor", dataVendors);
    AddCommand ("/vendor", VendorCmd);
    AddCommand("/vendordebug",VendorDebug);
    pVendorsType = new MQ2VendorsType;
}

PLUGIN_API VOID ShutdownPlugin (VOID)
{
    DebugSpewAlways ("Shutting down MQ2Vendors");
    WriteChatf ("MQ2Vendors::ShutdownPlugin");
    RemoveCommand ("/vendor");
    RemoveCommand("/vendordebug");
    RemoveMQ2Data("Vendor");
    ClearSearchList ();
    delete pVendorsType;
}

PLUGIN_API VOID OnPulse(VOID)
{
	static int sMWND = 0;
	static int skipPulses = 0;
	int mHeader = 0;
	int i, j, k;
	static int sListDone = 0;
	int hasItems = 0;
	CHAR sItem[MAX_STRING] = { 0 };
	i = j = k = 0;
	if (!InGames()) {
		sMWND = 0;
		sListDone = 0;
		skipPulses = 0;
		return;
	}
	if (!WindowOpen("MerchantWnd")) {
		sMWND = 0;
		sListDone = 0;
		skipPulses = 0;
		ItemData.clear();
		return;
	}
	CListWnd *cLWnd = NULL;
	//check our items list. we wait 200 pulses or end if we have MAX_VENDOR_SLOTS items.
	CXWnd *Win = pMerchantWnd->GetChildItem("MW_PurchasePage");
	if (Win) {
		if (CXWnd *Win2 = Win->GetChildItem("MW_ItemList")) {
			cLWnd = (CListWnd*)((CSidlScreenWnd*)(Win->GetChildItem("MW_ItemList")));
		}
		else {
			if (DEBUGGING)
				WriteChatf("MQ2Vendors: Could not get MW_ItemList.");
			return;
		}
	}
	else {
		if (DEBUGGING)
			WriteChatf("MQ2Vendors: Could not get MW_PurchasePage.");
		return;
	}
	if (!sListDone) {
		int c = 0;
		for (i = 0; i < MAX_VENDOR_SLOTS; i++) {
			CXStr cxstr = *cLWnd->GetItemText(&cxstr, i, 1);
			GetCXStr(cxstr.Ptr, sItem, MAX_STRING);
			if (strlen(sItem) > 0) {
				c++;
			}
		}
		if (c == MAX_VENDOR_SLOTS || skipPulses++ > SKIP_PULSES) {
			sListDone = 1;
		}
		return;
	}
	if (!sMWND) {
		CHAR Quantity[32] = { 0 };
		CHAR ZoneName[MAX_STRING] = { 0 };
		CHAR MerchantName[MAX_STRING] = { 0 };
		CHAR nItem[7] = { 0 };
		list <string>::iterator pList;
		_ItemData tItem;
		sMWND = 1;
		if (DEBUGGING) {
			WriteChatf("MQ2Vendors::Merchant Window is open");
			WriteChatf("MQ2Vendors: waited %d pulses", skipPulses);
		}
		strcpy_s(ZoneName, pZoneInfo ? ((PZONEINFO)pZoneInfo)->ShortName : "unknown");
		if (DEBUGGING)
			WriteChatf("Zone: %s", ZoneName);
		//update the merchants name(show whether they have items we want)
		GetCXStr(pMerchantWnd->GetFirstChildWnd()->CGetWindowText(), MerchantName, MAX_STRING);
		if (DEBUGGING) {
			WriteChatf("cLWnd type: %d UI_Listbox", ((CXWnd *)cLWnd)->GetType());
			WriteChatf("pMW name  : %s", MerchantName);
		}
		if (((CXWnd *)cLWnd)->GetType() == UI_Listbox) {
			//0 - icon
			// 1 - item name
			// 2 - merchant quantity(--means always have it)
			// 3 - plat - Now the currency icon
			// 4 - gold - now the cost section
			// 5 - silver - now the level section
			// 6 - copper - no longer used. 
			hasItems = 0;
			ItemData.clear();
			for (i = 0; i < MAX_VENDOR_SLOTS; i++) {
				CXStr cxstr = *cLWnd->GetItemText(&cxstr, i, 1);
				GetCXStr(cxstr.Ptr, sItem, MAX_STRING);
				if (DEBUGGING)
					WriteChatf("GetItemText(%d,1): %s", i, sItem);
				if (strlen(sItem) > 0) {
					//get the quantity for this     item slot.
					CXStr qtystr = *cLWnd->GetItemText(&qtystr, i, 2);
					GetCXStr(qtystr.Ptr, Quantity, MAX_STRING);
					//Start of Changes - Chatwiththisname 1/4/2019
					//These ints appear to store cP=Plat, cG=Gold, cS=Silver, cC=copper, need to parse the string in section 4 to get this now.
					int cP = 0, cG = 0, cS = 0, cC = 0;
					char cTemp[MAX_STRING] = { 0 };
					CXStr pCStr = *cLWnd->GetItemText(&pCStr, i, 4);
					GetCXStr(pCStr.Ptr, cTemp, MAX_STRING);
					//Convert the char array to an integer and multiply it by 1000. Had to add 1 to get that last copper to be correct.
					int iTemp = (int)(atof(cTemp) * 1000) + 1;
					cP = iTemp / 1000;
					cG = (iTemp - (cP * 1000)) / 100;
					cS = (iTemp - ((cP * 1000) + (cG * 100))) / 10;
					cC = (iTemp - ((cP * 1000) + (cG * 100) + (cS * 10))) / 1;
					//The below block of code is the old way. Writing a new way.
				   /* CXStr pCStr = *cLWnd->GetItemText(&pCStr, i, 3);
					GetCXStr(pCStr.Ptr, cTemp, MAX_STRING);
					cP = atoi(cTemp);
					CXStr gCStr = *cLWnd->GetItemText(&gCStr, i, 4);
					GetCXStr(gCStr.Ptr, cTemp, MAX_STRING);
					cG = atoi(cTemp);
					CXStr sCStr = *cLWnd->GetItemText(&sCStr, i, 5);
					GetCXStr(sCStr.Ptr, cTemp, MAX_STRING);
					cS = atoi(cTemp);
					CXStr cCStr = *cLWnd->GetItemText(&cCStr, i, 6);
					GetCXStr(cCStr.Ptr, cTemp, MAX_STRING);
					cC = atoi(cTemp);*/
					//End of Changes Chatwiththisname 1/4/2019
					//if quantity == '--', item is always on the merchant.
					if (Quantity && Quantity[0] == '-'
						&& Quantity[1] == '-') {
						//we now have an item that the merchant has upon server reboot.
						if (iSaveVendors) {
							WritePrivateProfileSection(MerchantName, "", MDBINIFile);
							if (!mHeader) {
								WritePrivateProfileString(MerchantName, "Zone", ZoneName, MDBINIFile);
								mHeader = 1;
							}
							sprintf_s(nItem, "Item%02d", j++);
							WritePrivateProfileString(MerchantName, nItem, sItem, MDBINIFile);
						}
					}
					else {
						sprintf_s(nItem, "Temp%02d", k++);
						if (DEBUGGING)
							WriteChatf("%s=%s", nItem, sItem);
						//does an item exist on our searchlist ?
						pList = SearchList.begin();
						while (pList != SearchList.end()) {
							if (!_stricmp(pList->c_str(), sItem)) {
								hasItems++;
								strcpy_s(tItem.Name, sItem);
								tItem.Quantity = atoi(Quantity);
								tItem.Plat = cP;
								tItem.Gold = cG;
								tItem.Silver = cS;
								tItem.Copper = cC;
								tItem.Slot = i;
								ItemData.push_back(tItem);
								WriteChatf("MQ2Vendors: >>> \ap%s\ax X \ar%s\ax(\atCost\ax: %dp %dg %ds %dc) \atItemSlot\ax: \ar%i", sItem, Quantity, cP, cG, cS, cC, i + 1);
								pList = SearchList.end();
							}
							else {
								pList++;
							}
						}
					}
				}
				else {
					if (DEBUGGING)
						WriteChatf("pMerch->ItemDesc[%02d] was empty", i);
				}
			}
			//update the merchants name(show whether they have items we want)
			if (hasItems) {
				CHAR Temp[MAX_STRING] = { 0 };
				CHAR Temp2[MAX_STRING] = { 0 };
				GetCXStr(pMerchantWnd->GetFirstChildWnd()->CGetWindowText(), Temp, MAX_STRING);
				if (strncmp(Temp, ">", 1)) {
					sprintf_s(Temp2, "> %s <", Temp);
					pMerchantWnd->GetFirstChildWnd()->CSetWindowText(Temp2);
					//SetCXStr((PCXSTR *)& pMerchantWnd->GetFirstChildWnd()->CGetWindowText(), Temp2);
				}
				WriteChatf("MQ2Vendors: %s has \ar%d\ax item%s we are searching for!",
					Temp, hasItems, hasItems > 1 ? "s" : "");
			}
			else {
				CHAR Temp[MAX_STRING] = { 0 };
				GetCXStr(pMerchantWnd->GetFirstChildWnd()->CGetWindowText(), Temp, MAX_STRING);
				WriteChatf("MQ2Vendors: %s has nothing we are searching for!", Temp);
			}
		}
	}
}
