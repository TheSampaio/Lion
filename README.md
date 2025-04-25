# Lion Engine

This is a 2D game engine made in C++ for my personal use. The project is being developed in my spare time to improve my skills as a software engineer. I combined its architecture with Judson Santiago's game engine, Yan Chernikov's game engine, and of course, my preference.

![Image](.github/image/lion-engine-logo-transparent.png)

## Build

Want to build and run Lion Engine? Just follow these steps:

1. **Install [Premake5](https://premake.github.io):**  
   Make sure `Premake` is installed and available in your system's PATH.

2. **Clone the repository with submodules:**  
   Use the following command to clone the project along with its submodules:  
   ``` sh
   git clone --recursive https://github.com/TheSampaio/Lion
   ```  
   
   If you cloned the repository without submodules, run:  
   ``` sh
   git submodule update --init
   ```

3. **Generate project files:**  
   From the root folder, run the appropriate [Premake command for your IDE](https://premake.github.io/docs/Using-Premake). For example:  
   ``` sh
   premake5 vs2022
   ```

4. **Open and build the project:**  
   Open the generated project with your IDE of choice, build it, and you're good to go!

## Credits

I would like to thank Judson and Cherno for the amazing free content available on their YouTube channel. If you want to learn more about computer graphics and game engine development, they certainly will help you. You can access their GitHub or YouTube channel below.

**Judson Santiago**  
• GitHub: [JudsonSS](https://github.com/JudsonSS)  
• YouTube: [@JudSan](https://www.youtube.com/@JudSan/featured)  
• Project: [Volt Engine](https://github.com/JudsonSS/Volt-Engine)

**Yan Chernikov**  
• GitHub: [TheCherno](https://github.com/TheCherno)  
• YouTube: [@TheCherno](https://www.youtube.com/@TheCherno)  
• Project: [Hazel Engine](https://github.com/TheCherno/Hazel)

## License

This project is licensed under the Apache License 2.0. You are free to use, modify, and distribute this project, but you must credit the original author and include a copy of the license. For more details, see the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).
