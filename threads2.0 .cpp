#include <filesystem>
#include <future>
#include <iostream>
#include <vector>
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace fs = std::filesystem;

class FILE_SEARCHER {
 public:
  void get_num_of_files_recursively_for(const fs::path& sub_dir);
  void search(int thread_number);
  const fs::path& get_file_name() const { return m_file_name; }
  void set_file_name(const fs::path& file_name) { m_file_name = file_name; }
  bool file_was_found() const { return m_file_found; }
  void stop_all_threads() { m_stop_all_threads = true; m_cv.notify_all(); }

 private:
  fs::path m_file_name;
  bool m_file_found = false;
  bool m_stop_all_threads = false;
  std::mutex m_mutex;
  std::condition_variable m_cv;
};

void FILE_SEARCHER::get_num_of_files_recursively_for(const fs::path& sub_dir) {
  unsigned int count = 0;

  for (const auto& entry : fs::recursive_directory_iterator(sub_dir)) {
    if (fs::is_regular_file(entry)) {
      ++count;
    }
  }

  std::cout << "Number of files in directory " << sub_dir << ": " << count << "\n";
}

void FILE_SEARCHER::search(int thread_number) {
  while (true) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() { return m_stop_all_threads || !m_file_name.empty(); });

    if (m_stop_all_threads) {
      std::cout << "Thread " << thread_number << " stopped." << std::endl;
      return;
    }

    bool file_found = false;

    for (auto& dir_entry : fs::recursive_directory_iterator(fs::current_path())) {
      if (dir_entry.is_directory()) {
        continue;
      }

      auto file_name = dir_entry.path().filename().string();
      if (file_name == m_file_name.string()) {
        std::cout << "file was found by thread " << thread_number << "! The name of file is " << file_name << std::endl;

        auto dir_path = dir_entry.path().parent_path();
        if (dir_entry.path().filename() == m_file_name) {
          std::cout << dir_path << std::endl;
        }

       
        std::cout << std::endl << "Counting amount of files in this current directory..." << std::endl << std::endl;

        get_num_of_files_recursively_for(dir_path);

        m_file_found = true;
        file_found = true;
        break;
      }
    }

    if (!file_found) {
      std::cout << "File not found by thread " << thread_number << "." << std::endl;
      stop_all_threads();
      return;
    }
  }
}



int main() {
  FILE_SEARCHER fs;

  fs.set_file_name("HAHA.txt");

  std::vector<std::future<void>> futures;
  for (int i = 1; i <= 5; ++i) {
    futures.emplace_back(std::async(std::launch::async, &FILE_SEARCHER::search, &fs, i));
  }

  for (auto& future : futures) {
    future.wait();
  }

  return 0;
}
