#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main()
{
   std::ios::sync_with_stdio(false);

   std::string command;
   std::string position;
   while (std::getline(std::cin, command))
   {
      if (command == "uci")
      {
         std::cout << "id name LittleBlitzer scripted fixture\n"
                   << "id author Nullstar test fixture\n"
                   << "uciok\n"
                   << std::flush;
      }
      else if (command == "isready")
      {
         std::cout << "readyok\n" << std::flush;
      }
      else if (command.rfind("position ", 0) == 0)
      {
         position = command;
      }
      else if (command.rfind("go", 0) == 0)
      {
         std::string move = "g6g7";
         if (position.find("7k/5K2/6Q1") == std::string::npos)
         {
            const std::vector<std::string> opening = {
               "e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6"
            };
            std::size_t move_count = 0;
            const std::size_t moves_at = position.find(" moves ");
            if (moves_at != std::string::npos)
            {
               std::istringstream moves(position.substr(moves_at + 7));
               std::string played;
               while (moves >> played)
                  ++move_count;
            }
            if (move_count < opening.size())
               move = opening[move_count];
         }

         std::cout << "bestmove " << move << "\n" << std::flush;
      }
      else if (command == "quit")
      {
         break;
      }
   }

   return 0;
}
