#pragma once
#include <string>

namespace Core {
namespace Commands {

class Command {
public:
    virtual ~Command() = default;
    
    // Executes the command (also used for redo)
    virtual void execute() = 0;
    
    // Reverts the command
    virtual void undo() = 0;
    
    // Optional: string name for UI display
    virtual std::string getName() const = 0;
};

} // namespace Commands
} // namespace Core
