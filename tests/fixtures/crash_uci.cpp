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
         std::cout << "id name LittleBlitzer crash fixture\n"
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
         // Die after using more than half the available second. This catches
         // comparisons against the post-search remaining time.
         std::this_thread::sleep_for(std::chrono::milliseconds(600));
         return 23;
      }
      else if (command == "quit")
      {
         break;
      }
   }

   return 0;
}
