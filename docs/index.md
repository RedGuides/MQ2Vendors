---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2vendors.206/"
support_link: "https://www.redguides.com/community/threads/mq2vendors.66898/"
repository: "https://github.com/RedGuides/MQ2Vendors"
config: "MQ2Vendors.ini"
authors: "ChatWithThisName"
tagline: "A plugin that lets you setup a search list of items that you want to be notified about if they popup on a vendor that your browsing."
quick_start: "https://www.redguides.com/community/resources/mq2vendors.206/"
---

# MQ2Vendors

<!--desc-start-->
MQ2Vendors is a plugin that lets you setup a search list of items that you want to be notified about if they popup on a vendor that your browsing. 
Notification happens in the MQ chat window and also by adding &lt;&lt;&lt;&lt; &gt;&gt;&gt;&gt; around the vendors name in the MerchantWnd.
<!--desc-end-->

## Commands

<a href="cmd-vendor/">
{% 
  include-markdown "projects/mq2vendors/cmd-vendor.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2vendors/cmd-vendor.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2vendors/cmd-vendor.md') }}

<a href="cmd-vendordebug/">
{% 
  include-markdown "projects/mq2vendors/cmd-vendordebug.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2vendors/cmd-vendordebug.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2vendors/cmd-vendordebug.md') }}

## Settings

The file MQ2Vendors.ini contains the plugin configuration, including the list of items to search for;

Example;

```ini
[ItemsList]
Item1=bone chips
```

## Top-Level Objects

## [Vendor](tlo-vendor.md)
{% include-markdown "projects/mq2vendors/tlo-vendor.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2vendors/tlo-vendor.md') }}

## DataTypes

## [Vendor](datatype-vendor.md)
{% include-markdown "projects/mq2vendors/datatype-vendor.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2vendors/datatype-vendor.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2vendors/datatype-vendor.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2vendors/datatype-vendor.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}
