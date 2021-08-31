package math

fun Sqrt(x Float64) -> Float64 {
    ret ccall<Float64>("Sqrt", x)
}
