#include <unicode/utypes.h>
#include <unicode/ucsdet.h>
#include <string>
#include <iostream>

#define BUFFER_SIZE 8192

inline int detect_charset(const std::string& sample,std::string* name = nullptr,std::string* lang = nullptr,int32_t* confidence = nullptr)
{
    UCharsetDetector* csd;
    const UCharsetMatch *csm;
    UErrorCode status = U_ZERO_ERROR;
    int32_t inputLength = (int32_t)sample.size();
    csd = ucsdet_open(&status);
    ucsdet_setText(csd, sample.data(), inputLength, &status);

    //return best match
    csm = ucsdet_detect(csd,&status);
    int32_t conf = ucsdet_getConfidence(csm,&status);
    std::cout << conf << std::endl;
    if(conf < 50)
    {
        ucsdet_close(csd);
        return -1; //poor detect
    }
    else {
        if(name)
            (*name) = ucsdet_getName(csm,&status);
        if(lang)
            (*lang) = ucsdet_getLanguage(csm,&status);
        if(confidence)
            *confidence = conf;
        ucsdet_close(csd);
        return 0;
    }

    // return orderd list of possible result
    // csm = ucsdet_detectAll(csd, &matchCount, &status);

}

#undef BUFFER_SIZE