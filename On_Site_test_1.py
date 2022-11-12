SIC_Program = list()
current_Address = 0
start_Address = int()
SYMTAB = dict() # string, int

Opcode = { #SIC模式下理論上opcode不會被動到 故直接存字串
    "Add": "18",
    "AND": "40",
    "COMP": "28",
    "DIV": "24",
    "J": "3C",
    "JEQ": "30",
    "JGT": "34",
    "JLT": "38",
    "JSUB": "48",
    "LDA": "00",
    "LDCH": "50",
    "LDL": "08",
    "LDX": "04",
    "MUL": "20",
    "OR": "44",
    "RD": "D8",
    "RSUB": "4C",
    "STA": "0C",
    "STCH": "54",
    "STL": "14",
    "STSW": "E8",
    "STX": "10",
    "SUB": "1C",
    "TD": "E0",
    "TIX": "2C",
    "WD": "DC"
}

class SIC_Line: #指令類別
    def __init__(self, line: str):
        print(line)
        self.address_Label, self.mnemonic_Opcode = line.removesuffix('\n').split('\t')[:2] #address_Label, mnemonic_Opcode, operand
        self.operands = line.removesuffix('\n').split('\t')[2] if len(line.removesuffix('\n').split('\t')) > 2 else ""
        self.object_Code = str()
        self.address = 0
        self.isDirectives = True; # 是否為組譯器指引(虛指令)

    def cal_Size(self): #計算大小同時生成虛指令的物件碼
        global start_Address
        self.length = 0
        match self.mnemonic_Opcode:
            case 'START':
                start_Address = int(self.operands, 16)
                self.address = start_Address
                self.object_Code = 'H' + self.address_Label.ljust(6, ' ') + f'{start_Address:x}'.rjust(6, '0')
                self.length = int(self.operands, 16)
            case 'END':
                self.object_Code = 'H' + f'{start_Address:x}'.rjust(6, '0')
                self.length = 0
            case 'BYTE':
                if self.operands.startswith('X'):
                    self.object_Code = self.operands[2:-1]
                if self.operands.startswith('C'):
                    self.object_Code = self.operands[2:-1].encode().hex()
                self.length = len(self.object_Code) // 2
            case 'WORD':
                self.object_Code = str(int(self.operands)).zfill(6)
                self.length = 3
            case 'RESB':
                self.length = int(self.operands)
            case 'RESW':
                self.length = int(self.operands) * 3
            case _:
                #實指令皆為3byte (format 1/2/4指令 只存在於XE)
                self.isDirectives = False
                return 3
        return self.length #不應該執行到這裡
    
    def pass2(self):
        if self.isDirectives:
            return
        addressing_Mode = 0      
        if self.operands != '' and self.operands.endswith(',X'):
            addressing_Mode = 0x8000
            self.operands = self.operands.removesuffix(',X')
        self.object_Code = Opcode[self.mnemonic_Opcode] + hex(SYMTAB[self.operands] + addressing_Mode).rjust(4, '0').removeprefix('0x')
    
    def pass1(self): #計算位置存入SYSTAB 處理虛指令
        global current_Address
        self.address = current_Address
        current_Address += self.cal_Size() #下一個指令的起始位置
        if self.address_Label != '':
            SYMTAB.update({self.address_Label: self.address})
        
if __name__ == "__main__":

    with open("input1.txt", "r") as f:
        SIC_Program = [SIC_Line(i) for i in f.readlines() if not i.startswith('.')] #將每一行的字串轉換成list並存入SIC_Program中,並直接移除註解部分
        
    #執行pass1計算loc以及SYMTAB
    for i in SIC_Program:
        i.pass1()
    SYMTAB.update({'': 0}) #空字串代表沒有label
    for i in SIC_Program:
        i.pass2()   
    
    with open("output1.txt", "w") as f:
        f.write(SIC_Program[0].object_Code + f'{(current_Address - start_Address):x}'.rjust(6, '0').upper() + '\n')
        
        line_Start_Address = -1; line_Object_Code = str()
        
        for i in range(1, len(SIC_Program)-1):
            print(SIC_Program[i].object_Code)
            if SIC_Program[i].object_Code == '':
                continue #如果為空則跳過
            if line_Start_Address == -1:
                line_Start_Address = SIC_Program[i].address #紀錄該行開頭的位置
                
            line_Object_Code += SIC_Program[i].object_Code
            if (len(line_Object_Code) + len(SIC_Program[i+1].object_Code)) > 60 or SIC_Program[i+1].object_Code == '' or SIC_Program[i+1].mnemonic_Opcode == 'END': #若長度達到極限(加上下一筆超過60個字元) 或是下一行並無物件碼/為END則輸出
                f.write('T' + hex(line_Start_Address).removeprefix('0x').rjust(6, '0').upper() + hex(len(line_Object_Code)//2).removeprefix('0x').rjust(2, '0').upper() + line_Object_Code.removeprefix('0x').upper() + '\n')
                line_Start_Address = -1
                line_Object_Code = str()
                
        f.write(SIC_Program[-1].object_Code.upper())