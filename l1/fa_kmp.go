package main

import (
	"fmt"
	"os"
	"strings"
)

func computePrefixFunction(pattern string) []int {
	m := len(pattern)
	pi := make([]int, m)
	k := 0
	for q := 1; q < m; q++ {
		for k > 0 && pattern[k] != pattern[q] {
			k = pi[k-1]
		}
		if pattern[k] == pattern[q] {
			k++
		}
		pi[q] = k
	}
	return pi
}

func KMPSearch(text, pattern string) {
	n, m := len(text), len(pattern)
	pi := computePrefixFunction(pattern)
	q := 0
	for i := 0; i < n; i++ {
		for q > 0 && pattern[q] != text[i] {
			q = pi[q-1]
		}
		if pattern[q] == text[i] {
			q++
		}
		if q == m {
			fmt.Printf("Pattern occurs with shift %d\n", i-m+1)
			q = pi[q-1]
		}
	}
}

func buildAutomaton(pattern string) []map[byte]int {
	m := len(pattern)
	transitionFunction := make([]map[byte]int, m+1)
	for i := range transitionFunction {
		transitionFunction[i] = make(map[byte]int)
	}
	for q := 0; q <= m; q++ {
		for i := 0; i < 256; i++ {
			a := byte(i)
			k := q
			for k > 0 && (k == m || pattern[k] != a) {
				k = k - 1
			}
			if k < m && pattern[k] == a {
				k++
			}
			transitionFunction[q][a] = k
		}
	}
	return transitionFunction
}

func FASearch(text, pattern string) {
	transitionFunction := buildAutomaton(pattern)
	n, m := len(text), len(pattern)
	q := 0
	for i := 0; i < n; i++ {
		q = transitionFunction[q][text[i]]
		if q == m {
			fmt.Printf("Pattern occurs with shift %d\n", i-m+1)
		}
	}
}

func main() {
	if len(os.Args) != 4 {
		fmt.Println("Usage: <algorithm> <pattern> <filename>")
		return
	}

	algorithm := os.Args[1]
	pattern := os.Args[2]
	filename := os.Args[3]

	data, err := os.ReadFile(filename)
	if err != nil {
		fmt.Printf("Error reading file: %v\n", err)
		return
	}

	text := string(data)

	switch strings.ToUpper(algorithm) {
	case "KMP":
		KMPSearch(text, pattern)
	case "FA":
		FASearch(text, pattern)
	default:
		fmt.Println("Unknown algorithm. Use 'KMP' or 'FA'")
		return
	}
}
