#include <iostream>
#include<iomanip>
#include<fstream>
#include<string>
#include<sstream>
#include<vector>
#include<map>
using namespace std;
class SIC_Line;
int current_Address=0;
int start_Address;
ofstream output("output.txt", ios::out);
ofstream loc("loc.txt", ios::out);
vector<SIC_Line> SIC_Program;
map<string, int> SYMTAB;
map<string, string> Opcode = {
    {"ADD","18"},{"AND","40"},{"COMP","28"},{"DIV","24"},{"J","3C"},{"JEQ","30"},{"JGT","34"},{"JLT","38"},{"JSUB","48"},{"LDA","00"},{"LDCH","50"},{"LDL","08"},{"LDX","04"},{"MUL","20"},{"OR","44"},{"RD","D8"},{"RSUB","4C"},{"STA","0C"},{"STCH","54"},{"STL","14"},{"STSW","E8"},{"STX","10"},{"SUB","1C"},{"TD","E0"},{"TIX","2C"},{"WD","DC"}
};//SIC模式下理論上opcode不會被動到 故直接存字串
class SIC_Line
{
public:
    string address_Label = "";
    string mnemonic_Opcode = "";
    string operands = "";
    string object_Code="";
    int address=0;
    bool isDirectives = true;//是否為組譯器指引(虛指令)
    SIC_Line(string line)
    {
        stringstream ss;
        ss << line;
        getline(ss, address_Label, '\t');
        getline(ss, mnemonic_Opcode, '\t');
        getline(ss, operands, '\t');
    }
    void print()//偵錯用
    {
        cout<< setw(4) << setfill('0') << hex << address;
        cout << "\t" << address_Label << "\t" << mnemonic_Opcode << "\t" << operands << "\t" << object_Code << endl;
    }
    void pass1()//計算位置存入SYSTAB 處理虛指令
    {
        stringstream ss;
        address = current_Address;
        current_Address += cal_Size();//下一個指令的起始位置
        if (address_Label != "")
        {
            SYMTAB[address_Label]=address;
        }
        loc << uppercase << setw(4) << setfill('0') << hex << address<<"\t";
        loc << address_Label << "\t" << mnemonic_Opcode << "\t" << operands << endl;
    }
    int cal_Size()//計算大小同時生成虛指令的物件碼
    {
        //先判斷虛指令 start直接將起始位置視為size做加法
        if (mnemonic_Opcode == "START")
        {
            stringstream ss;
            start_Address= stoi(operands, 0, 16);
            address = start_Address;
            ss << "H"<< setw(6) << setfill(' ') << left<<address_Label<< setw(6) << setfill('0') << hex<<right << start_Address;
            getline(ss,object_Code);
            return stoi(operands, 0, 16);//將operands以16進制解讀 並回傳/載入到第二個參數位置中(未使用到故填入NULL(0))
        }
        if (mnemonic_Opcode == "END")
        {
            stringstream ss;
            ss << "E" << setw(6) << setfill('0') << hex << start_Address;
            ss >> object_Code;
            return 0;
        }
        if (mnemonic_Opcode == "BYTE")
        {
            if (operands[0] == 'X')
            {
                for (int i = 2; i < operands.length() - 1; i++)object_Code += operands[i];
            }
            if (operands[0] == 'C')
            {
                stringstream ss;
                for (int i = 2; i < operands.length() - 1; i++)
                {
                    ss << uppercase << setw(2) << hex << setfill('0') << (int)operands[i];
                }
                ss >> object_Code;
            }
            return object_Code.length() / 2;
        }
        if (mnemonic_Opcode == "WORD")
        {
            stringstream ss;
            ss << uppercase << hex<< setw(6) << setfill('0')  <<stoi(operands);
            ss >> object_Code;
            return 3;
        }
        if (mnemonic_Opcode == "RESB")
        {
            return stoi(operands);
        }
        if (mnemonic_Opcode == "RESW")
        {
            return stoi(operands) * 3;
        }
        //實指令皆為3byte (format 1/2/4指令 只存在於XE)
        isDirectives = false;
        return 3;
    }
    void pass2()
    {
      
        if(isDirectives==false)
        {
            int addressing_Mode = 0;
            if (operands!="" && operands[operands.length() - 2] == ',')//若倒數第二個為','(,X結尾 不會出現其他例外) 將索引定址模式設為1(0x8000) 
            {
                stringstream ss;
                ss << operands;
                getline(ss, operands, ',');//進行分割 去除",X"
                addressing_Mode = 0x8000;
            }
            stringstream ss;
            ss << hex << uppercase << setw(4) << setfill('0') << (SYMTAB[operands]+addressing_Mode);//先將address欄位輸出
            ss >> object_Code;
            object_Code = Opcode[mnemonic_Opcode]+ object_Code;//再拼上opcode
        }
        output <<uppercase<< setw(4) << setfill('0') << hex << address << "\t";//輸出 output.txt
        output << address_Label << "\t" << mnemonic_Opcode << "\t" << operands;
        if (!isDirectives || mnemonic_Opcode == "BYTE" || mnemonic_Opcode == "WORD")output << "\t" << object_Code;//只有實指令 BYTE WORD 需要輸出物件碼
        output << endl;
    }
};
int main()
{
    ifstream input_File("input.txt",ios::in);
    if (input_File.is_open())
    {
        string input_Line;
        while (getline(input_File, input_Line))
        {
            if(input_Line[0]!='.')SIC_Program.push_back(SIC_Line(input_Line));//非註解則輸入
        }
        input_File.close();
    }
    for (auto& L : SIC_Program)L.pass1();//執行pass1計算loc以及SYMTAB
    for (auto& L : SIC_Program)L.print();//偵錯用
    for (auto& L : SIC_Program)L.pass2();
    for (auto& L : SIC_Program)L.print();//偵錯用
    ofstream objectcode("objectcode.txt", ios::out);
    if (objectcode.is_open())
    {
        objectcode << uppercase << SIC_Program[0].object_Code << hex << setw(6) << setfill('0') << current_Address - start_Address << endl;//輸出H紀錄 分別為( H+程式名稱(6格寬)+起始位置 )+ 總程式長度 前者已於PASS1完成
        //T紀錄為 T+起始位置+長度(BYTE)+物件碼
        int line_Start_Address = -1;
        string line_Object_Code = "";
        for (int i = 1; i < SIC_Program.size() - 1; i++)
        {
            if (SIC_Program[i].object_Code == "")continue;//如果為空則跳過
            if (line_Start_Address == -1)line_Start_Address = SIC_Program[i].address;//若為開頭(尚無紀錄) 則紀錄該行開頭的位置
            line_Object_Code += SIC_Program[i].object_Code;//串接物件碼
            if (line_Object_Code.length() +SIC_Program[i+1].object_Code.length()>60 || SIC_Program[i + 1].object_Code == "" || SIC_Program[i + 1].mnemonic_Opcode == "END")//若長度達到極限(加上下一筆超過60個字元/30BYTE) 或是下一行並無物件碼/為END則輸出 
            {
                objectcode << "T" << uppercase << setw(6) << setfill('0') << hex << line_Start_Address << setw(2) << setfill('0') << hex << line_Object_Code.length() / 2 << line_Object_Code<<endl;
                //重置資料
                line_Start_Address = -1;
                line_Object_Code = "";
            }
        }
        objectcode << uppercase << SIC_Program[SIC_Program.size() - 1].object_Code;//輸出E紀錄
        objectcode.close();
        output.close();
        loc.close();
    }
    cin.get();
    return 0;
}
