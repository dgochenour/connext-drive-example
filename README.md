# Working Connext Drive:  </br>Static Endpoint Discovery and Admin Console

## General Overview

This example demonstrates interoperability between the "out of the box" Connext Drive Evaluation `-exampleTemplate cert` example and RTI's Admin Console tool. Because a `cert` system uses static endpoint discovery (DPSE), some configuration is required on both the Admin Console side, and on the stock example side.

Section 5 of the Connext Drive Eval Documentation describes how a user would create the starting point for this repo's example:
```
$RTIMEHOME/rtiddsgen/scripts/rtiddsgen -micro -example -exampleTemplate cert -language C HelloWorld.idl
```
This command should be called in this repo's top-level directory to generate type-support files. Note that existing example source files will not be overwritten.

### Changes To The C Example
- In `HelloWorldApplication.h`, `USE_ADMINCONSOLE` can be defined (by default, it is) to configure "Admin Console-friendly" resource limits and assertions.
### Changes To Admin Console's Configuration
- Guidance and files related to the configuration of Admin Console can be found in this repo's [admin_console_config directory](./admin_console_config/).

## Compiling With `rtime-make`

The following assumptions are made:

* The environment variable `RTIMEHOME` is set to the Connext Drive Eval installation directory 
* Libraries exist in your installation for the architecture in question
    * For example `x64Linux4gcc7.3.0_certprofile` 

### Linux on Intel x64: 

    $ cd your/project/directory 
    $ $RTIMEHOME/resource/scripts/rtime-make --config Release --build --target x64Linux4gcc7.3.0_certprofile --source-dir . -G "Unix Makefiles" --delete

After the build completes, the executables can be found in the `objs/x64Linux4gcc7.3.0_certprofile` directory.

## Running example_publisher and example_subscriber

Open 2 terminal sessions. Start the publisher in one terminal and the subscriber in the second. 

    $ ./objs/x64Linux4gcc7.3.0_certprofile/HelloWorld_publisher 
    
Run the second subscriber in another terminal with the command:

    $ ./objs/x64Linux4gcc7.3.0_certprofile/HelloWorld_subscriber  


