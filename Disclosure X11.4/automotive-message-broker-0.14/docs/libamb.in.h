/*!
 \file libamb.h
 \section libamb Automotive Message Broker Library Documentation
 \version @PROJECT_VERSION@

 \section libamb_intro Introduction
 Automotive Message Broker (AMB) Library documentation outlines the internal classes and structures for building
 plugins for AMB.

 \section libamb_architecture General Architecture
 AMB has 3 main parts.  Source plugins which provide data, a routing engine that
 routes data and sink plugins that consume the data.

 \section libamb_properties Properties
 AMB defines a number of properties itself.  These properties are defined in vehicleproperty.h.  The DBus plugin
 will take many of these properties and combine them in DBus interfaces.  The mappings of AMB internal properties
 to DBus Interface properties can be found in the <a href="ambdbusmappings_8idl.html">mappings documentation</a>.  This file will come in handy when you want to
 implement a particular AMB DBus interface in your source plugin.

 By default, for any property not explicitly
 included in a DBus interface, the DBus plugin will generate a custom interface.  The pattern is as follows:

 CustomProperty = "org.automotive.CustomProperty.CustomProperty"

 "org.automotive.CustomProperty is the DBus interface and CustomProperty is a DBus property in that interface.

 \section libamb_plugins Plugins
 There are two types of plugins: plugins that provide data, called "sources"
 (AbstractSource) and plugins that consume data, called "sinks" (AbstractSink).
 A typical source would get data from the vehicle and then translate the raw data
 into AMB property types.  Sinks then subscribe to the property types and do useful
 things with the data.

 Example plugins can be found in plugins/exampleplugin.{h,cpp} for an example
 source plugin and plugins/examplesink.{h,cpp} for an example sink plugin.  There
 are also many different types of plugins useful for testing and development in the
 plugins/ directory.

 Various plugins have separate documentation found in @DOC_INSTALL_DIR@/plugins/.

 \section libamb_plugin_creation Creating your own plugin
 AMB allows you to create your own plugins.  Plugins inherit from either AbstractSource, AbstractSink, or AmbPluginImpl.

 It is recommended that new plugins be written using AmbPlugin and AmbPluginImpl.

 \section routing_engine Routing Engine Plugins
 As of 0.12, the routing engine itself can be exchanged for a plugin.  This allows
 users to swap in routing engines with different behaviors, additional security,
 and custom throttling and filtering features.

 The easiest way to get started creating a routing engine plugin would be to look at
 AbstractRoutingEngine, the base class for all routing engines and the default
 routing engine in ambd/core.cpp.
**/
