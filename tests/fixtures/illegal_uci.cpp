#include <iostream>
#include <string>

int main()
{
   std::ios::sync_with_stdio(false);

   std::string command;
   while (std::getline(std::cin, command))
   {
      if (command == "uci")
      {
         std::cout << "id name LittleBlitzer illegal-move fixture\n"
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
         std::cout << "bestmove a1a8\n" << std::flush;
      }
      else if (command == "quit")
      {
         break;
      }
   }

   return 0;
}
