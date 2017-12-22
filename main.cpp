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

template<class Ehdr, class Phdr, class Shdr>
void do_segment_info_printing(vector<Phdr> &ptable) {
  printSegments<Phdr>(ptable);
}

template<class Phdr, class Shdr>
void do_projection_relation_printing(const vector<Phdr> &ph, const vector<Shdr> &sh) {
  map<uint64_t, vector<string>> hash;
  for(auto & item : ph){
    hash[item.]
  }
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
  do_segment_info_printing<Ehdr, Phdr, Shdr>(phtable);

  //relation info
  do_projection_relation_printing<Phdr, Shdr>(phtable, shtable);
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