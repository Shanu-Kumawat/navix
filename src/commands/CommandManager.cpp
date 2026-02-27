#include "commands/CommandManager.hpp"
#include <iostream>

namespace Core {
namespace Commands {

void CommandManager::executeCommand(std::unique_ptr<Command> command) {
    if (!command) return;
    
    command->execute();
    addCommand(std::move(command));
}

void CommandManager::addCommand(std::unique_ptr<Command> command) {
    if (!command) return;
    
    undoStack.push_back(std::move(command));
    redoStack.clear(); // Whenever a new action occurs, redo history is lost
}

bool CommandManager::undo() {
    if (undoStack.empty()) return false;
    
    auto command = std::move(undoStack.back());
    undoStack.pop_back();
    
    command->undo();
    redoStack.push_back(std::move(command));
    return true;
}

bool CommandManager::redo() {
    if (redoStack.empty()) return false;
    
    auto command = std::move(redoStack.back());
    redoStack.pop_back();
    
    command->execute();
    undoStack.push_back(std::move(command));
    return true;
}

bool CommandManager::canUndo() const {
    return !undoStack.empty();
}

bool CommandManager::canRedo() const {
    return !redoStack.empty();
}

void CommandManager::clear() {
    undoStack.clear();
    redoStack.clear();
}

} // namespace Commands
} // namespace Core
