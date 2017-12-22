#include <elf.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <map>

using namespace std;

bool isValid(const vector<char> &content) {
  //check if this is a elf file. only 'unsigned char e_ident[EI_NIDENT];' here
  if (content[EI_MAG0] != ELFMAG0 ||
      content[EI_MAG1] != ELFMAG1 ||
      content[EI_MAG2] != ELFMAG2 ||
      content[EI_MAG3] != ELFMAG3) {
    return false;
  }

  if (content[EI_CLASS] != ELFCLASS32 && content[EI_CLASS] != ELFCLASS64) {
    return false;
  }
  if (content[EI_DATA] != ELFDATA2LSB && content[EI_DATA] != ELFDATA2MSB) {
    return false;
  }
  if (content[EI_VERSION] != EV_CURRENT) {
    return false;
  }

  return true;
}
template<class Phdr>
void printSegments(const vector<Phdr> &ptable) {
  cout << "segment info:" << endl;
  int count = 0;
  for (auto &item : ptable) {
    cout << "No. " << count << "\t";
    count++;
    if (item.p_type == PT_NULL) {
      cout << "NULL segment" << endl;
      continue;
    }
    cout << hex << "vaddr(hex):" << setw(16) << item.p_vaddr << " mem size(hex):" << setw(16) << item.p_memsz << dec
         << endl;
  }
  cout << "segment info ends" << endl;
}

template<class Shdr>
void printSections(const vector<Shdr> &stable, const char *names) {
  cout << "section info:" << endl;
  int count = 0;

  for (auto &item : stable) {
    cout << "No. " << count << "\t";
    count++;
    cout << setw(20);
    if (item.sh_type == SHT_NULL) {
      cout << "NULL section" << endl;
      continue;
    }
    if (names) {
      cout << names + item.sh_name << " ";
    }
    cout << hex << "vaddr(hex) : " << setw(16) << item.sh_addr << " size(hex):" << setw(16) << item.sh_size << dec
         << endl;
  }
  cout << "section info ends." << endl;
}

template<class Ehdr, class Shdr>
void extractShtableAndNames(Ehdr &header,
                            Shdr &initial,
                            const vector<char> &content,
                            vector<Shdr> &shtable,
                            const char *&names) {
  if (header.e_shoff == 0) {
    cout << "this file contains no section header info" << endl;
    return;
  }
  uint32_t shtableSize = header.e_shnum;
  if (header.e_shnum == SHN_LORESERVE) {
    shtableSize = initial.sh_size;
  }

  shtable.resize(shtableSize);
  copy(content.begin() + header.e_shoff,
       content.begin() + header.e_shoff + header.e_shnum * header.e_shentsize,
       (char *) &shtable[0]);

  names = nullptr;
  if (header.e_shstrndx == SHN_UNDEF) {
    cout << "not shstr table. thus no section names" << endl;
  } else {
    names = &content[shtable[header.e_shstrndx].sh_offset];
    if (header.e_shstrndx == SHN_XINDEX) {
      int index = initial.sh_link;
      names = &content[shtable[index].sh_offset];
    }

  }
}

template<class Shdr>
void do_section_header_printing(vector<Shdr> &shtable,
                                const char *&names) {
  printSections<Shdr>(shtable, names);
}

template<class Ehdr, class Phdr, class Shdr>
void extractPhtable(Ehdr &header,
                    Shdr &initial,
                    const vector<char> &content,
                    vector<Phdr> &ptable) {
  if (header.e_phoff == 0) {
    cout << "this file contains no program header info" << endl;
    return;
  }
  uint32_t ptableSize = header.e_phnum;
  if (header.e_phnum == PN_XNUM) {
    ptableSize = initial.sh_info;
  }

  ptable.resize(ptableSize);
  copy(content.begin() + header.e_phoff,
       content.begin() + header.e_phoff + header.e_phentsize * header.e_phnum,
       (char *) &ptable[0]);
}

template<class Phdr, class Shdr>
void do_projection_relation_printing(const vector<Phdr> &ph, const vector<Shdr> &sh, const char *names) {
  cout << "segment and section mapping relation" << endl;
  if (ph.empty() || sh.empty()) {
    cout << "no projection relation due to insufficient ph or sh info" << endl;
    return;
  }
  vector<vector<string>> result(ph.size());
  for (int i = 0; i < sh.size(); i++) {
    if (sh[i].sh_type == SHT_NULL) {
      continue;
    }
    string n = names ? names + sh[i].sh_name : to_string(i);
    uint64_t addr = sh[i].sh_addr;
    for (int j = 0; j < ph.size(); j++) {
      if (ph[j].p_type == PT_NULL) {
        continue;
      }
      uint64_t lower = ph[j].p_vaddr;
      uint64_t upper = ph[j].p_vaddr + ph[j].p_memsz;
      if (addr >= lower && addr < upper) {
        result[j].push_back(n);
      }
    }
  }

  for (int i = 0; i < result.size(); i++) {
    cout << setw(4) << i;
    cout << "\t";
    for (auto &item : result[i]) {
      cout << item << " ";
    }
    cout << endl;
  }

//  map<uint64_t, vector<string>> hash;
//  for (auto &item : ph) {
//    hash[item.p_vaddr] = {};
//  }
//  for (int i = 0; i < sh.size(); i++) {
//    auto &item = sh[i];
//    auto iter = lessOrEqual<uint64_t, vector<string>>(hash, item.sh_addr);
//    if (iter != hash.end()) {
//      iter->second.push_back(names ? names + item.sh_name : to_string(i));
//    } else {
//      cerr << "shaddr:" << item.sh_addr << " " << endl;
//      assert(false);
//    }
//  }
//
//  int count = 0;
//  for (auto &item : hash) {
//    cout << setw(4) << "No. " << count << " ";
//    count++;
//    cout << setw(4) << item.first << "\t";
//    for (auto &inner : item.second) {
//      cout << inner << " ";
//    }
//    cout << endl;
//  }
//  cout << endl;
  cout << "segment and section mapping relation ends" << endl;
}
template<class Ehdr, class Phdr, class Shdr>
void print_info(const vector<char> &content) {
  Ehdr header;

  copy(content.begin(), content.begin() + sizeof(header), (char *) &header);
  cout << "file type:" << header.e_type << "REL:1 EXEC:2 DYN:3 CORE:4" << endl;

  /**
   * <del>it's not likely to run into those huge ph or sh files. fix this if needed in actual usage.</del>
   *
   */
  Shdr initial;
  copy(content.begin() + header.e_shoff,
       content.begin() + header.e_shoff + header.e_shentsize,
       (char *) &initial);

  vector<Shdr> shtable;
  const char *names;
  extractShtableAndNames(header, initial, content, shtable, names);
  do_section_header_printing(shtable, names);

  //program header info
  vector<Phdr> phtable;
  extractPhtable(header, initial, content, phtable);
  printSegments<Phdr>(phtable);
  //mapping info
  do_projection_relation_printing<Phdr, Shdr>(phtable, shtable, names);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "specify only one argument as object file" << endl;
    return 0;
  }
  ifstream ifs(argv[1], ios::binary);
  vector<char> content((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());

  if (!isValid(content)) {
    cout << argv[1] << " is not valid elf file" << endl;
    return 0;
  }

  if (content[EI_CLASS] == ELFCLASS32) {
    cout << "32bit elf file" << endl;
    print_info<Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr>(content);
  } else {
    cout << "64 bit elf file" << endl;
    print_info<Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr>(content);
  }

  return 0;
}