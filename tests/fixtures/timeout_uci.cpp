#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main()
{
   std::ios::sync_with_stdio(false);

   std::string command;
   while (std::getline(std::cin, command))
   {
      if (command == "uci")
      {
         std::cout << "id name LittleBlitzer timeout fixture\n"
                   << "id author Nullstar test fixture\n"
                   << "uciok\n"
                   << std::flush;
      }
      else if (command == "isready")
      {
         std::cout << "readyok\n" << std::flush;
      }
      else if (command.rfind("go", 0) == 0)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(1500));
         std::cout << "bestmove 0000\n" << std::flush;
      }
      else if (command == "quit")
      {
         break;
      }
   }

   return 0;
}
