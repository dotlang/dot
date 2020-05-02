package main

import "os"


func main() {
    file, _ := os.Create("out.bin")
    defer file.Close()

    b := []byte{0x63,0x91}
    file.Write(b)
}
