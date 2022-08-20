package main

import (
	"bufio"
	"fmt"
	"os"
)

func readLinesFromIO(f *os.File, counts map[string]int, fileMap map[string]string) {
	input := bufio.NewScanner(f)
	for input.Scan() { // 每次调用Scan()会读取下一行并且移除换行符，保存的内容可以通过input.Text()拿到，并且在没有碰见EOF前返回true
		counts[input.Text()]++
		fileMap[input.Text()] = f.Name()
	}
}
func main() {
	counts := make(map[string]int) // hash map . coutns是一个引用类型
	fileMap := make(map[string]string)
	files := os.Args[1:]
	if len(files) == 0 {
		readLinesFromIO(os.Stdin, counts, fileMap)
	} else {
		for _, arg := range files {
			f, err := os.Open(arg)
			if err != nil {
				fmt.Fprintf(os.Stderr, "dup2:%v\n", err) // format and print to io handle
				continue
			} else {
				readLinesFromIO(f, counts, fileMap)
				f.Close()
			}
		}

	}
	for line, n := range counts {
		if n > 1 {
			fmt.Printf("%d\t%s\t%s\n", n, line, fileMap[line])
		}
	}
}
