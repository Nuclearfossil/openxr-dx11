# Overview
In this section, we're actually looking at code. You can find all the code in the `OpenXR-Tutorial-001` Visual Studio project. In it, we will be reviewing the basic initialization of OpenXR, as well as covering in more details the elementary aspects of OpenXR. This project only displays information, to the console, about features available to you in OpenXR. We do not render anything to device.

# Our Project
## Project Setting
I like to have all my additional library includes and lib files in a globally accessible place. Thus, I tend to use package managers like `VCXPROJ` or `Conan` to manage all that. However, I don't want to force the reader to use one of those package managers, so I create two folders called `include` and `libs` to store those additional files:
![[Pasted image 20230210085201.png]]
![[Pasted image 20230210085219.png]]
If you're more comfortable using package managers, that's a far easier way to go.

In `OpenXR-Tutorial-001`, our project is set up to be a standard Windows Console app. It is not a true Windows app (no message pump, or windows creation events). All information we want will be written to the console, through a third party library called `EasyLogging` [github repo](https://github.com/amrayn/easyloggingpp). 

Initialization of Easylogging is pretty easy. Simply add the C++ source to your project, add the `easylogging++.h` header file, and add this macro to you source code in one spot:
``` C++
	INITIALIZE_EASYLOGGING\
```
And you're ready to start using it.

It's a data stream, like `std::cout`, where you can stream data to. So doing something like:
``` C++
LOG(INFO) << "You done screwed up " << (1 + 1);
```
will end up streaming to the console:
```
2023-02-09 21:52:01,583 INFO [default] You done screwed up 2
```
It will also generate a file called `myeasylog.log` to your disk, so you can always see what happened during each run.

editors note: easylogging is a bit of an older project. I may end up replacing it with something that's a bit more maintained in a future revision.

## Include Files and Set up
Aside from the include file for easylogging, you'll next need the header file for OpenXR. I've got a `libs` and 'include' global folder for additional libraried, including libs I've created for OpenXR. All the OpenXR headers live in an `openxr` folder, so my includes look like this:

``` C++
#include <windows.h>

#include "openxr\openxr.h"
#include "openxr\openxr_platform.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP\

#include <vector>
#include <iostream>
```
If you've done any windows development, this will look familiar. Technically you do not need the include for `openxr_platform.h`, but we will be using it shortly, so I like to keep it for completeness.

Additionally, I also need to include libs into my project dependency for OpenXR. Here's my settings for that:
![[Pasted image 20230210085502.png]]
![[Pasted image 20230210085524.png]]

## The Main Loop

We can start off with something fairly trivial, like the following:
``` C++
int main()
{
	LOG(INFO) << "Successful OpenXR Tutorial Run";
	return 0;
}
```

Compiling and running this example will just spew the `Successful OpenXR Tutorial Run` text to the console, like so:

```
2023-02-10 09:01:55,206 INFO [default] Successful OpenXR Tutorial Run
```

If you've got a successfully compiling and running program at this point in time, we can now move on to adding OpenXR to this code.

## Determining Available OpenXR Extensions
In [[02 - Project Overview - OpenXR#^e6f366|this]] section, we discussed OpenXR extensions. What we would like to do next is actually determine what extensions we have available to us. To do that, we need to enumerate those extensions. It's actually really simple to do, through the `xrEnumerateInstanceExtensionProperties` OpenXR function. This will query the OpenXR runtime to determine what extensions are available, by name.

The pattern that we follow to use this function is:
- call the function with only one (or two) parameters so we can determine how many enumeratable objects there are, so we can allocate the appropriate number of buffers to hold the data.
- call the function again, but with the allocated buffers that the OpenXR function can put data into.

You'll see this pattern very often when enumerating OpenXR resources.

# Summary

# Resources
- [Easy Logging Github Repo](https://github.com/amrayn/easyloggingpp) - Github Repo for EasyLogging lib
- 