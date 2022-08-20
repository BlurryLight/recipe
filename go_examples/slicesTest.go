package main

import "fmt"

func RemoveSliceAtindex(s []string, index int) []string {
	return append(s[:index], s[index+1:]...) // ...操作符可以解开slice以传递给变参函数
}
func main() {
	s := make([]string, 3)
	fmt.Println("empty: ", s)

	s[0] = "a"
	s[1] = "b"
	s[2] = "c"
	fmt.Printf("init abc %v \t %d\n: ", s, len(s))

	s = append(s, "e")
	s = append(s, "d")
	s = append(s, "f")
	fmt.Printf("append %v \t %d\n: ", s, len(s))

	copyS := make([]string, len(s))
	copy(copyS, s)
	fmt.Printf("copyS %v \t %d\n: ", copyS, len(copyS))

	//test slice

	l := s[2:5]
	fmt.Println("l: ", l)

	s1 := RemoveSliceAtindex(s, 0) // should be b c d e d f
	fmt.Println("s1: ", s1)

}
