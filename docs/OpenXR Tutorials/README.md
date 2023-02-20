# Intent

In an effort to familiarize myself with OpenXR (and the core foundations of how it works) I thought I would put together a little tutorial for myself and others on how one acutally uses OpenXR. The point of this will be education, not performance. However as part of it being an educational process, I will be attempting 'best practices' in building out how this work.

I will also try as much as possible to keep the codebase lean and clean. Don't expect a lot of 
'modern' C++ features in the codebase. Using core features of the STL will probably be the extent of how 'modern' the codebase will be.

# API and Platform Choices

OpenXR is the predominant API for this tutorial. We will need a graphics API to work with and I've chosen DirectX 11. I find it a nice compromise between lower level access to the hardware but at the same time being general purpose enough to be less 'wordy' than an api like vulkan. I'm not a graphics programmer, so the graphics code isn't performant by any stretch.

In the future, I may look at other graphics APIs (OpenGL, DirectX 12, potentially Vulkan), but that requires more of an investment than I can make currently.

As well, as the graphics API of choice is DirectX, we will be working on a Windows PC, depolying to a Rift-style device. For me, it is a Quest 2 / Quest Pro through a Link cable. That said, the concepts I'll review will be general enough that they should be transferrable to your platform of choice.

# Tutorial Structure

I'll be breaking down the topics to cover as succintly as is possible. I would rather create 100 short pages than one 100000 line page.

Where appropriate, I will link to the code, as well as inline as much code to illustrate the concepts as I can.

Each page will start with an overview, the main body of the lesson, and a final summary to wrap up concepts.

# Tutorial Steps
- [[01 - Project Overview - DirectX 11]] - Where I type a lot of words
- [[02 - Project Overview - OpenXR]] - Where I type even more words
- [[03 - Basic Initialization - Part 1]] - Where the Code starts to come alive (OpenXR-Tutorial-001)
	- [[03.01 - Why OpenXR Structs Have to redefine their type]]
- [[04 - Basic Initialization - Part 2]] - Where we get another step closer to a running bit of code (OpenXR-Tutorial-002)
- [[05 - Where we get all the details]] - Almost there (OpenXR-Tutorial-003)
 


