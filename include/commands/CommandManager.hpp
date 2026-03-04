#pragma once
#include "Command.hpp"
#include <vector>
#include <memory>

namespace Core {
namespace Commands {

class CommandManager {
public:
    CommandManager() = default;
    ~CommandManager() = default;

    // Executes a new command and adds it to the undo stack
    void executeCommand(std::unique_ptr<Command> command);

    // Adds a command to the history without executing it
    // Useful for commands that were executed during interactive drag
    void addCommand(std::unique_ptr<Command> command);

    bool undo();
    bool redo();

    bool canUndo() const;
    bool canRedo() const;

    void clear();

private:
    std::vector<std::unique_ptr<Command>> undoStack;
    std::vector<std::unique_ptr<Command>> redoStack;
};

} // namespace Commands
} // namespace Core
