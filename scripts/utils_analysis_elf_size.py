# coding=utf8
import sys
import os

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection




class ElfSizeInfo(object):
    def __init__(self):
        self.code_size = 0
        self.rodata_size = 0
        self.data_size = 0
        self.bss_size = 0
        self.bss_pfb_size = 0

    # def get_analysis_data(self):
    #     info_str = ''
    #     info_str += ("\n")
    #     info_str += ("\n")
    #     info_str += ("| section                     | type                | size(Bytes)         | size(hex)           |\n")
    #     info_str += ("| -------------------         | ------------------- | ------------------- | ------------------- |\n")
    #     info_str += ("| code_size                   | text                |%21s|%21s|\n" % (self.code_size, hex(self.code_size)))
    #     info_str += ("| ram_size                    | data&bss            |%21s|%21s|\n" % (self.data_size+self.bss_size, hex(self.data_size+self.bss_size)))
    #     info_str += ("\n")
    #     info_str += ("\n")


    #     return info_str



def utils_process_elf_file(filename):
    # print('Processing file:', filename)

    elf_size_info = ElfSizeInfo()

    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        # for section in elffile.iter_sections():
        #     print('name: %s' % section.name)
        #     print('header: %s' % section.header)

        # Just use the public methods of ELFFile to get what we need
        # Note that section names are strings.
        # print('  %s sections' % elffile.num_sections())
        section = elffile.get_section_by_name('.symtab')

        if not section:
            print('  No symbol table found. Perhaps this ELF has been stripped?')
            return

        # A section type is in its header, but the name was decoded and placed in
        # a public attribute.
        # print('  Section name: %s, type: %s' %(section.name, section['sh_type']))

        # But there's more... If this section is a symbol table section (which is
        # the case in the sample ELF file that comes with the examples), we can
        # get some more information about it.
        if isinstance(section, SymbolTableSection):
            num_symbols = section.num_symbols()
            # print("  It's a symbol section with %s symbols" % num_symbols)
            
            # 遍历打印 节区头入口
            for symbol in section.iter_symbols():
                # print('name:', symbol.name)
                # print('header', symbol.entry)
                # print('st_value', symbol['st_value'])

                if symbol.name == '__code_size':                    
                    elf_size_info.code_size = symbol['st_value']
                elif symbol.name == '__rodata_size':
                    elf_size_info.rodata_size = symbol['st_value']
                elif symbol.name == '__data_size':
                    elf_size_info.data_size = symbol['st_value']
                elif symbol.name == '__bss_size':
                    elf_size_info.bss_size = symbol['st_value']
                elif symbol.name == '__bss_pfb_size':
                    elf_size_info.bss_pfb_size = symbol['st_value']
    return elf_size_info




def get_example_list():
    # path = '../../../example'
    path = 'example'
    app_list = []

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path):
            app_list.append(file)
    
    return app_list

def get_example_basic_list():
    path = 'example/HelloBasic'
    app_list = []

    files = os.listdir(path)
    for file in files:
        file_path = os.path.join(path, file)

        if os.path.isdir(file_path):
            app_list.append(file)
    
    return app_list


def compile_code(params):
    # compile code
    cmd = 'make clean'
    res = os.system(cmd)
    if res != 0:
        return res
    cmd = 'make all -j' + params
    print(cmd)
    res = os.system(cmd)
    if res != 0:
        return res

    return 0


def process_app(current_work_cnt, total_work_cnt, app, app_basic, params):
    print("=================================================================================")
    print("Total Work Cnt: %d, Current Cnt: %d, Process: %.2f%%" 
        % (total_work_cnt, current_work_cnt, current_work_cnt * 100.0 / total_work_cnt))
    print("=================================================================================")

    if app_basic != None:
        params_full = params + (' PORT=stm32g0_empty APP=%s APP_SUB=%s') % (app, app_basic)
    else:
        params_full = params + (' PORT=stm32g0_empty APP=%s') % (app)
    res = compile_code(params_full)
    if(res != 0):
        sys.exit(res)
        
    elf_size_info = utils_process_elf_file('output/main.elf')
    
    # info = elf_size_info.get_analysis_data()
    # print(info)
    app_name = app
    if app_basic != None:
        app_name = app + '(' + app_basic + ')'
    return ("|%24s|%21s|%21s|%21s|%21s|\n" % (app_name, elf_size_info.code_size, elf_size_info.rodata_size
                                                    , elf_size_info.data_size + elf_size_info.bss_size - elf_size_info.bss_pfb_size, elf_size_info.bss_pfb_size))


if __name__ == '__main__':
    #params = ' V=1'
    params = ''

    app_sets = get_example_list()
    app_basic_sets = get_example_basic_list()

    total_work_cnt = 0
    for app in app_sets:
        if app == "HelloBasic":
            for app_basic in app_basic_sets:
                total_work_cnt += 1
        else:
            total_work_cnt += 1


    info_str = ''
    info_str += ("\n")
    info_str += ("\n")
    info_str += ("| app                    | Code(Bytes)         | Resource(Bytes)     | RAM(Bytes)          | PFB(Bytes)          |\n")
    info_str += ("| ---------------------- | ------------------- | ------------------- | ------------------- | ------------------- |\n")

    current_work_cnt = 0
    for app in app_sets:
        if app == "HelloBasic":
            for app_basic in app_basic_sets:
                current_work_cnt += 1
                info_str += process_app(current_work_cnt, total_work_cnt, app, app_basic, params)
        else:
            current_work_cnt += 1
            info_str += process_app(current_work_cnt, total_work_cnt, app, None, params)
    
    with open('output/README.md', 'w', encoding='utf-8') as f:
        f.write(info_str)
    
    print(info_str)

    sys.exit(0)
