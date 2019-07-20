#ifndef DUMMYFILESYSTEM_H
#define DUMMYFILESYSTEM_H

#include "daemon/filesystem.h"
#include <catch.hpp>
#include <set>

struct FileNode
{
  FileNode(std::string_view name, bool is_dir, uint64_t mtime)
  {
    entry.name = name;
    entry.is_dir = is_dir;
    entry.mtime = mtime;
  }

  fw::dm::FileSystem::DirectoryEntry entry;
  std::unique_ptr<FileNode> child;
  std::unique_ptr<FileNode> sibling;
};

class DummyFileSystem : public fw::dm::FileSystem
{
public:
  DummyFileSystem(uint64_t root_mtime=12345) : root("/", true, root_mtime) {}

  FileNode root;

  void add_dir(std::string_view containing_dirname,
               std::string_view dirname,
               uint64_t mtime)
  {
    auto node = find_node(containing_dirname);
    REQUIRE(node != nullptr);
    insert_child(*node, std::make_unique<FileNode>(dirname, true, mtime));
  }

  FileNode* find_node(std::string_view name)
  {
    REQUIRE(name[0] == '/');
    name = name.substr(1);
    auto* current = &root;
    FileNode* parent = nullptr;
    while (current != nullptr && !name.empty())
    {
      auto pos = name.find_first_of('/');
      if (pos == std::string_view::npos)
        pos = name.size() - 1;

      parent = current;
      current = nullptr;
      auto curname = name.substr(0, pos + 1);
      name = name.substr(pos + 1);
      auto child = parent->child.get();
      while (child != nullptr)
      {
        if (child->entry.name == curname)
        {
          current = child;
          break;
        }
        child = child->sibling.get();
      }
    }
    return current;
  }

  const FileNode* find_node(std::string_view name) const
  {
    REQUIRE(name[0] == '/');
    name = name.substr(1);
    auto* current = &root;
    const FileNode* parent = nullptr;
    while (current != nullptr && !name.empty())
    {
      auto pos = name.find_first_of('/');
      if (pos == std::string_view::npos)
        pos = name.size() - 1;

      parent = current;
      current = nullptr;
      auto curname = name.substr(0, pos + 1);
      name = name.substr(pos + 1);
      auto child = parent->child.get();
      while (child != nullptr)
      {
        if (child->entry.name == curname)
        {
          current = child;
          break;
        }
        child = child->sibling.get();
      }
    }
    return current;
  }

  void insert_child(FileNode& parent, std::unique_ptr<FileNode> node)
  {
    if (parent.child == nullptr)
    {
      parent.child = std::move(node);
    }
    else
    {
      FileNode* current = nullptr;
      auto* next = parent.child.get();
      while (next != nullptr && next->entry.name < node->entry.name)
      {
        current = next;
        next = next->sibling.get();
      }
      if (current == nullptr)
      {
        node->sibling = std::move(parent.child);
        parent.child = std::move(node);
      }
      else
      {
        node->sibling = std::move(current->sibling);
        current->sibling = std::move(node);
      }
    }
  }

  void add_file(std::string_view containing_dirname,
                std::string_view filename,
                uint64_t mtime)

  {
    auto node = find_node(containing_dirname);
    REQUIRE(node != nullptr);
    insert_child(*node, std::make_unique<FileNode>(filename, false, mtime));
  }

  std::deque<DirectoryEntry> ls(std::string_view entryname) const override
  {
    auto node = find_node(entryname);
    if (!node)
      return std::deque<DirectoryEntry>{};

    std::deque<DirectoryEntry> entries{};
    if (!node->entry.is_dir)
      entries.push_back(node->entry);
    else
    {
      auto child = node->child.get();
      while (child != nullptr)
      {
        entries.push_back(child->entry);
        child = child->sibling.get();
      }
    }
    return entries;
  }

  std::optional<DirectoryEntry>
  get_direntry(std::string_view entryname) const override
  {
    auto node = find_node(entryname);
    if (!node)
      return std::optional<DirectoryEntry>();

    return node->entry;
  }

  bool exists(std::string_view filename) const override
  {
    return find_node(filename) != nullptr;
  }

  bool isdir(std::string_view dirname) const override
  {
    auto node = find_node(dirname);
    return node != nullptr && node->entry.is_dir == true;
  }

};

#endif /* DUMMYFILESYSTEM_H */
