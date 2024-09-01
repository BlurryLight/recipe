# test UTF-8

## Build

### Windows
VCPKG is recommmended for install dependencies.
`libicu` and `iconv` are needed. 
`libicu` is employed to detect string encoding.
`iconv` is employed to convert string to utf-8.

```
vcpkg install icu iconv
```

### Linux

Use vcpkg or `apt` or `pacman` or whatever to install `libiconv` and `libicu`.

## Test
- Display well in code page 65001(UTF-8).
![README-2022-03-25-18-52-10](https://img.blurredcode.com/img/README-2022-03-25-18-52-10.png?x-oss-process=style/compress)

- output UTF-8 string. In shell (cmd/powershell) with Activate Code Page 936, it will be scrambled code.
![README-2022-03-25-18-53-58](https://img.blurredcode.com/img/README-2022-03-25-18-53-58.png?x-oss-process=style/compress)

