---
tags:
  - datatype
---
# `Vendor`

<!--dt-desc-start-->
Returns information about the current vendor's items.
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='float', name='Version') }}

:   Returns the version number of the plugin

### {{ renderMember(type='bool', name='HasItems') }}

:   Whether the vendor has any items you are looking for.

### {{ renderMember(type='string', name='Items[#].Name') }}

:   Specify the index of the item, and it will return the item name.  
    e.g. `${Vendor.Items[1].Name}`

### {{ renderMember(type='int', name='Count') }}

:   The number of items the vendor has that you are searching for, does not currently work.

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[float]: ../macroquest/reference/data-types/datatype-float.md
[int]: ../macroquest/reference/data-types/datatype-int.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
