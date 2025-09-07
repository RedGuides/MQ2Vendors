---
tags:
  - command
---

# /vendor

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/vendor <option> <string>
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Configuration of the MQ2Vendor plugin to setup a search list of items that you want to be notified about when browsing a vendor.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `savevendors` | Enables saving vendor base items. |
| `nosavevendors` | Disables saving vendor base items. |
| `list` | Lists the current items being watched for. |
| `found` | Lists items found on the current merchant from your list. |
| `add <string>` | Adds parameter to the search list. &lt;string&gt; can be an item link, MQ2Vendors will remove the extra link data before saving the item name. |
| `remove <string>` | Removes parameter from the search list. &lt;string&gt; can be an item link, MQ2Vendors will remove the extra link data before saving the item name. |
