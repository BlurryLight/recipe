#include <iostream>
#include <fstream>
#include <string>
#include <iconv.h> //for gbk/big5/utf8
#include <string.h>
#include <vector>
//https://stackoverflow.com/questions/28270310/how-to-easily-detect-utf8-encoding-in-the-string
//take from here
bool is_valid_utf8(const char * string)
{
    if (!string)
        return true;

    const unsigned char * bytes = (const unsigned char *)string;
    unsigned int cp;
    int num;

    while (*bytes != 0x00)
    {
        if ((*bytes & 0x80) == 0x00)
        {
            // U+0000 to U+007F
            cp = (*bytes & 0x7F);
            num = 1;
        }
        else if ((*bytes & 0xE0) == 0xC0)
        {
            // U+0080 to U+07FF
            cp = (*bytes & 0x1F);
            num = 2;
        }
        else if ((*bytes & 0xF0) == 0xE0)
        {
            // U+0800 to U+FFFF
            cp = (*bytes & 0x0F);
            num = 3;
        }
        else if ((*bytes & 0xF8) == 0xF0)
        {
            // U+10000 to U+10FFFF
            cp = (*bytes & 0x07);
            num = 4;
        }
        else
            return false;

        bytes += 1;
        for (int i = 1; i < num; ++i)
        {
            if ((*bytes & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | (*bytes & 0x3F);
            bytes += 1;
        }

        if ((cp > 0x10FFFF) ||
            ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
            ((cp <= 0x007F) && (num != 1)) ||
            ((cp >= 0x0080) && (cp <= 0x07FF) && (num != 2)) ||
            ((cp >= 0x0800) && (cp <= 0xFFFF) && (num != 3)) ||
            ((cp >= 0x10000) && (cp <= 0x1FFFFF) && (num != 4)))
            return false;
    }

    return true;
}
//https://gist.github.com/mgttt/d0aba83da88552f95edafb65124d41c3
//take from here
int code_convert(char *from_charset,char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	iconv_t cd;
	int rc;
	char **pin = &inbuf;
	char **pout = &outbuf;
	cd = iconv_open(to_charset,from_charset);
	if (cd==0)
		return -1;
	memset(outbuf,0,outlen);
	if (iconv(cd,pin,&inlen,pout,&outlen) == -1)
		return -1;
	iconv_close(cd);
	return 0;
}
std::string any2utf8(std::string in,std::string fromEncode,std::string toEncode)
{
	char* inbuf=(char*) in.c_str();
	int inlen=strlen(inbuf);
	int outlen=inlen*3;//in case unicode 3 times than ascii
    // char outbuf[outlen]={0};
    std::vector<char> outbuf(outlen,0);
    outbuf.resize(outlen);

	int rst=code_convert((char*)fromEncode.c_str(),(char*)toEncode.c_str(),inbuf,inlen,outbuf.data(),outlen);
	if(rst==0){
		return std::string(outbuf.begin(),outbuf.end());
	}else{
		return in;
	}
}
std::string gbk2utf8(const char* in)
{
	return any2utf8(std::string(in),std::string("gbk"),std::string("utf-8"));
}

std::string gbk2utf8(const std::string& in)
{
    if(in.empty()) return "";
	return any2utf8(in,std::string("gbk"),std::string("utf-8"));
}
int main()
{
    std::string line;
    auto read_file = []{
    std::ifstream ifs("test_gbk.txt");
    while(!ifs.eof())
    {
        std::getline(ifs,line);
        // std::cout << line << std::endl;
        if(is_valid_utf8(line.c_str()))
        {
            std::cout << line << std::endl;
        }
        else
        {
            std::cout << "converted"<< std::endl;
            std::cout << gbk2utf8(line) << std::endl;
        }
    }
    }
   
    return 0;
}
