#ifndef DUMMYFILESYSTEM_H
#define DUMMYFILESYSTEM_H

#include "daemon/filesystem.h"

#include <catch2/catch.hpp>
#include <spdlog/spdlog.h>

#include <set>

struct FileNode
{
  FileNode(std::string_view name, bool is_dir, uint64_t mtime)
  {
    entry.name = name;
    entry.is_dir = is_dir;
    entry.mtime = mtime;
    entry.size = 0;
  }

  fw::dm::fs::DirectoryEntry entry;
  std::unique_ptr<FileNode> child;
  std::unique_ptr<FileNode> sibling;
  std::string contents;
};

class DummyFileSystem : public fw::dm::FileSystem
{
public:
  DummyFileSystem(std::string_view rootdir_, uint64_t root_mtime = 12345) :
    fw::dm::FileSystem(rootdir_), root("/", true, root_mtime)
  {
  }

  FileNode root;

  void add_dir(std::string_view containing_dirname,
               std::string_view dirname,
               uint64_t mtime)
  {
    auto node = find_node(containing_dirname);
    REQUIRE(node != nullptr);
    insert_child(*node, std::make_unique<FileNode>(dirname, true, mtime));
    spdlog::debug("add_dir({}, {}, {}) -> {} - {}", containing_dirname, dirname,
                  mtime, node->entry.name, dirname);
  }

  void rm_dir(std::string_view containing_dirname, std::string_view dirname)
  {
    auto node = find_node(containing_dirname);
    REQUIRE(node != nullptr);
    remove_child(*node, dirname, true);
    spdlog::debug("rm_dir({}, {}) -> {} - {}", containing_dirname, dirname,
                  node->entry.name, dirname);
  }

  FileNode* find_node(std::string_view name)
  {
    std::ostringstream ost;
    ost << "[m] find_node(" << name << ") -> ";
    REQUIRE(name[0] == '/');
    name = name.substr(1);
    auto* current = &root;
    FileNode* parent = nullptr;
    while (current != nullptr && !name.empty())
    {
      auto pos = name.find_first_of('/');
      if (pos == std::string_view::npos)
        pos = name.size();

      parent = current;
      current = nullptr;
      auto curname = name.substr(0, pos);
      ost << "/" << curname;
      name = name.substr(std::min(pos + 1, name.size()));
      auto child = parent->child.get();
      std::ostringstream ost2;
      ost2 << "[";
      while (child != nullptr)
      {
        ost2 << " " << child->entry.name;
        if (child->entry.name == curname)
        {
          current = child;
          break;
        }
        child = child->sibling.get();
      }
      ost2 << " ]";
      if (!current)
        ost << "? " << ost2.str();
    }
    if (current)
      ost << " OK";
    spdlog::debug(ost.str());
    return current;
  }

  const FileNode* find_node(std::string_view name) const
  {
    std::ostringstream ost;
    ost << "[c] find_node(" << name << ") -> ";
    REQUIRE(name[0] == '/');
    name = name.substr(1);
    auto* current = &root;
    const FileNode* parent = nullptr;
    while (current != nullptr && !name.empty())
    {
      auto pos_next_slash = name.find_first_of('/');
      if (pos_next_slash == std::string_view::npos)
        pos_next_slash = name.size();

      parent = current;
      current = nullptr;
      auto curname = name.substr(0, pos_next_slash);
      ost << "/" << curname;
      name = name.substr(std::min(pos_next_slash + 1, name.size()));
      auto child = parent->child.get();
      std::ostringstream ost2;
      ost2 << "[";
      while (child != nullptr)
      {
        ost2 << " " << child->entry.name;
        if (child->entry.name == curname)
        {
          current = child;
          break;
        }
        child = child->sibling.get();
      }
      ost2 << " ]";
      if (!current)
        ost << "? " << ost2.str();
    }
    if (current)
      ost << " OK";
    spdlog::debug(ost.str());
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

  void remove_child(FileNode& parent, std::string_view name, bool isdir)
  {
    FileNode* prev = nullptr;
    auto* c = &parent.child;
    while (*c != nullptr)
    {
      if ((*c)->entry.name == name && (*c)->entry.is_dir == isdir)
      {
        if (prev == nullptr)
        {
          parent.child = std::move((*c)->sibling);
        }
        else
        {
          prev->sibling = std::move((*c)->sibling);
        }
        return;
      }
      else
      {
        prev = c->get();
        c = &(*c)->sibling;
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
    spdlog::debug("add_file({}, {}, {}) -> {} - {}", containing_dirname,
                  filename, mtime, node->entry.name, filename);
  }

  void rm_file(std::string_view containing_dirname, std::string_view filename)
  {
    auto node = find_node(containing_dirname);
    REQUIRE(node != nullptr);
    remove_child(*node, filename, false);
    spdlog::debug("rm_file({}, {}) -> {} - {}", containing_dirname, filename,
                  node->entry.name, filename);
  }

  void write_file(std::string_view entryname, std::string_view contents)
  {
    auto node = find_node(entryname);
    REQUIRE(node != nullptr);
    node->contents = contents;
    node->entry.size = contents.size();
  }

  std::size_t size(std::string_view entryname)
  {
    auto node = find_node(entryname);
    REQUIRE(node != nullptr);
    return node->entry.size;
  }

  std::deque<fw::dm::fs::DirectoryEntry>
  ls(std::string_view entryname) const override
  {
    using fw::dm::fs::DirectoryEntry;

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

  std::optional<fw::dm::fs::DirectoryEntry>
  get_direntry(std::string_view entryname) const override
  {
    auto node = find_node(entryname);
    if (!node)
    {
      spdlog::debug("Could not find direntry: {}\n", entryname);
      return std::optional<fw::dm::fs::DirectoryEntry>();
    }
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

  std::string read(std::string_view filepath) const override
  {
    const auto* node = find_node(filepath);
    REQUIRE(node != nullptr);
    return node->contents;
  }

  void watch(std::string_view dirname,
             fw::dm::DirectoryEventListener& listener) override
  {
    listeners.push_back(std::make_pair(&listener, std::string(dirname)));
  }

  void stop_watching(std::string_view dirname,
                     fw::dm::DirectoryEventListener& listener) override
  {
    for (auto iter = std::begin(listeners); iter != std::end(listeners); ++iter)
      if (iter->first == &listener && iter->second == dirname)
      {
        listeners.erase(iter);
        return;
      }
  }

  std::vector<std::pair<fw::dm::DirectoryEventListener*, std::string>>
    listeners;
};

#endif /* DUMMYFILESYSTEM_H */
