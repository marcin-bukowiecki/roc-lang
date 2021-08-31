package main

fun adder (
a Int, b
Int) ->
Int {
    ret
    a +
    b
}

fun
box() -> Bool {
    if adder(12, 25) == 37 {
        ret true
    }
    ret false
}
