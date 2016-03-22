from subprocess import Popen, PIPE
import re
import time


class PMLX(object):

    @classmethod
    def pxe_to_virtual_page(cls, pxe):
        "pxe comes in this form: 67 d0 30 7b 00 00 00 00"
        try:
            octets = pxe.split()
            virtual = "0xffff8800" + octets[3] + octets[2] + octets[1][0] + "000"
            return virtual
        except:
            return pxe

    @property
    def is_null_entry(self):
        if self.entry == "00 00 00 00 00 00 00 00":
            return True
        return False

    @property
    def is_large_page(self):
        if self.entry[0] == "e":
            return True
        return False

    def __init__(self, level, pxe_entry):
        self.level = level
        self.entry = pxe_entry
        self.virtual_address = self.pxe_to_virtual_page(pxe_entry)
        self.pmlx_entries = []

    def __iter__(self):
        return iter(self.pmlx_entries)

class PageWalker(object):

    REGEX = r"\| (.+) \|"

    def __init__(self, tmp_filename):
        self.fw = open(tmp_filename, "wb")
        self.fr = open(tmp_filename, "r")
        self.p = Popen("./client", stdin=PIPE, stdout=self.fw, stderr=self.fw, bufsize=1)
        self.page_tree = None
        self.pxe_pattern = re.compile(self.REGEX)
        self._skip_menu()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        print "Error: ", traceback
        self.fw.close()
        self.fr.close()

    def _skip_menu(self):
       time.sleep(0.2)
       self.fr.read(0x1000)

    def find_common_frames(self, page_tree):
        tree1_entries = []
        tree2_entries = []

        print "Finding Common entries..."

        for pdpt in self.page_tree:
            tree1_entries.append("PDPT " + pdpt.virtual_address)
            for pd in pdpt:
                tree1_entries.append("PD   " + pd.virtual_address)
                for pt in pd:
                    tree1_entries.append("PT   " + pt.virtual_address)
                    for pte in pt:
                        tree1_entries.append("PTE  " + pte.virtual_address)

        for pdpt in page_tree:
            tree2_entries.append("PDPT " + pdpt.virtual_address)
            for pd in pdpt:
                tree2_entries.append("PD   " + pd.virtual_address)
                for pt in pd:
                    tree2_entries.append("PT   " + pt.virtual_address)
                    for pte in pt:
                        tree2_entries.append("PTE  " + pte.virtual_address)

        for entry in tree1_entries:
            if entry in tree2_entries:
                print "Common Entry: ", entry
            else:
                print "Uncommon Entry", entry

    def read_cr3(self):
        self.p.stdin.write("cr3\n")
        time.sleep(0.2)
        outline = self.fr.read()
        cr3 = next(_ for _ in outline.split() if _[0:2] == "0x")
        return hex(int(cr3,16) + 0xffff880000000000)[:-1]

    def read_pmlx_entries(self, pmlx_base, level, sleep_time, virtual_address, bytes):
        line = "r %s %x\n" % (virtual_address, bytes)
        print "LINE: %s - base: %s - level: %s" %(line[:-1], pmlx_base.virtual_address, level)
        self.p.stdin.write(line)
        time.sleep(sleep_time)
        output = self.fr.read()
        for match in self.pxe_pattern.finditer(output):
            line = match.group(1)
            entry = line[0:23]
            # if entry != "00 00 00 00 00 00 00 00":
            pmlx_base.pmlx_entries.append(PMLX(level=level, pxe_entry=entry))
            entry = line[24:]
            # if entry != "00 00 00 00 00 00 00 00":
            pmlx_base.pmlx_entries.append(PMLX(level=level, pxe_entry=entry))

    def _add_offset(self, base, offset):
        return hex(int(base,16) + offset)[:-1]

    def walk_structures(self, callback):
        f = open("output.txt", "wt")
        cr3 = self.read_cr3()
        self.page_tree = PMLX(level="PML4", pxe_entry=cr3)
        # Read PML4 Entries
        # with grsec only the first 8 entries are filled
        #print "CR3: \n", cr3
        f.write("CR3: %s \n" % cr3)
        self.read_pmlx_entries(self.page_tree, "PDPT", 1, cr3, 0x1000)
        covered_memory = 0
        for index_pml4e, pml4e in enumerate(self.page_tree):
            if index_pml4e == 0x110: # only dump PTEs for the autoref entry 0x110
                if not pml4e.is_null_entry:
                    #print "%s - PDPT: %s : %s\n" % (self._add_offset(cr3, index_pml4e*8), pml4e.entry, pml4e.virtual_address)
                    f.write("%s - PDPT: %s : %s\n" % (self._add_offset(cr3, index_pml4e*8), pml4e.entry, pml4e.virtual_address))
                    self.read_pmlx_entries(pml4e, "PD", 1, pml4e.virtual_address, 0x1000)

                    for index_pdpte, pdpte in enumerate(pml4e):
                        if not pdpte.is_null_entry:
                            #print "    %s - PD: %s : %s" % (self._add_offset(pml4e.virtual_address,index_pdpte*8), pdpte.entry, pdpte.virtual_address)
                            f.write("    %s - PD: %s : %s\n" % (self._add_offset(pml4e.virtual_address,index_pdpte*8), pdpte.entry, pdpte.virtual_address))
                            self.read_pmlx_entries(pdpte, "PT", 1, pdpte.virtual_address, 0x1000)
                            #if pml4e.virtual_address == "0xffff8800019f0000": # only dump PTEs for the autoref entry 0x110
                            for index_pde, pde in enumerate(pdpte):
                                if not pde.is_null_entry:
                                    if pde.is_large_page:
                                        covered_memory += 2048 # 2MB (in KB)
                                        #print "        %s - PT: %s : %s - %10d KB - %10d MB - %10d GB" % (self._add_offset(pdpte.virtual_address,index_pde*8), pde.entry, pde.virtual_address, covered_memory, covered_memory/1024, covered_memory/1048576)
                                        f.write("        %s - PT: %s : %s - %10d KB - %10d MB - %10d GB\n" % (self._add_offset(pdpte.virtual_address,index_pde*8), pde.entry, pde.virtual_address, covered_memory, covered_memory/1024, covered_memory/1048576))
                                        self.read_pmlx_entries(pde, "PT Entries", 1, pde.virtual_address, 0x1000)
                                        continue
                                    else:
                                        #print "        %s - PT: %s : %s" % (self._add_offset(pdpte.virtual_address,index_pde*8), pde.entry, pde.virtual_address)
                                        f.write("        %s - PT: %s : %s\n" % (self._add_offset(pdpte.virtual_address,index_pde*8), pde.entry, pde.virtual_address))
                                        self.read_pmlx_entries(pde, "PT Entries", 1, pde.virtual_address, 0x1000)
                                        for index_pte, pte in enumerate(pde):
                                            covered_memory += 4 # 4k
                                            if not pte.is_null_entry:
                                                #print "            %s - PT Entry: %s : %s - %10d KB - %10d MB - %10d GB" % (self._add_offset(pde.virtual_address, index_pte*8), pte.entry, pte.virtual_address, covered_memory, covered_memory/1024, covered_memory/1048576)
                                                f.write("            %s - PT Entry: %s : %s - %10d KB - %10d MB - %10d GB\n" % (self._add_offset(pde.virtual_address, index_pte*8), pte.entry, pte.virtual_address, covered_memory, covered_memory/1024, covered_memory/1048576))
                                else:
                                    covered_memory += 2048 # 2MB (in KB)
                        else:
                            covered_memory += 1048576 # 1GB (in KB)
                else:
                    covered_memory += 536870912 # 512GB (in KB)

        f.close()


if __name__ == "__main__":
    with PageWalker("tmpout") as pageWalker:        
        pageWalker.walk_structures(callback=None)

    #p1 = PageWalker("tmpout1")
    #p1.walk_structures(callback=None)
    #p2 = PageWalker("tmpout2")
    #p2.walk_structures(callback=None)

    #p2.find_common_frames(p1.page_tree)


