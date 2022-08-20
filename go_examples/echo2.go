package main

import (
	"fmt"
	"os"
	"strings"
)

func main() {
	s, seq := "", ""
	for index, arg := range os.Args[:] { // range每次产生index和value,类似python的enumerate
		s += seq + arg
		seq = " "
		fmt.Printf("%d %s\n", index, arg)
	}
	fmt.Println(s)
	fmt.Println(len(os.Args))

	fmt.Println(strings.Join(os.Args[:], " "))
	fmt.Println(os.Args[:])

	var d = true
	fmt.Println(d)

	short_string := "short"
	fmt.Println(short_string)

}
