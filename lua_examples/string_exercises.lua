s1 = [=[
"<![CDATA[
  Hello world
]]>"]=]
print(s1)
s2 = '\x22\x3C\x21\x5B\x43\x44\x41\x54\x41\x5B\x0A\x20\z
     \x20\x48\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64\x0A\x5D\x5D\x3E\x22'
print(s2)
print(s2 == s1)
s3 = '"<![CDATA[\n  Hello world\n]]>"'
print(s3)
print(s3 == s1)

-- Suppose you need to write a long sequence of arbitrary bytes as a literal string in Lua. 
-- ans: use [[ bytes ]] to avoid unexpected output

function insert(s,pos,another_s)
    prefix = string.sub(s,1,pos - 1) -- if sub(s,i,j)  j < i it returns empty
    suffix = string.sub(s,pos,-1)
    return prefix .. another_s .. suffix
end
print(insert("hello world",1,"start: "))
print(insert("hello world",7,"small "))

function insert_utf8(s,pos,another_s)
    prefix = string.sub(s,1,utf8.offset(s,pos - 1))
    suffix = string.sub(s,utf8.offset(s,pos),-1)
    return prefix .. another_s .. suffix
end
print(insert_utf8 ("ação", 5, "!"))


function remove(s,pos,length)
    prefix = string.sub(s,1,pos - 1)
    suffix = string.sub(s,pos + length,-1)
    return prefix .. suffix
end

function remove_utf8(s,pos,length)
    prefix = string.sub(s,1,utf8.offset(s,pos - 1))
    suffix = string.sub(s,utf8.offset(s,pos + length),-1)
    return prefix .. suffix
end
print(remove("hello world",7,4))
print(remove_utf8("ação",2,2))

function ispali(s)
    return s == string.reverse(s)
end

function ispali_plus(s)
    local new_s = string.gsub(s,"[%p%s]","")
    return new_s == string.reverse(new_s)
end

print(ispali("apple elppa"))
print(ispali("apple elppa."))
print(ispali_plus("apple elppa"))

function utf8_reverse(s)
    local new_s = ""
    for p,c in utf8.codes(s) do
        new_s = utf8.char(c) .. new_s
    end
    return new_s
end

function ispali_utf8(s)
    local new_s = string.gsub(s,"[%p%s]","")
    return new_s == utf8_reverse(new_s)
end

print(ispali_utf8("apple elppa"))
print(ispali_utf8("apple你好你elppa"))