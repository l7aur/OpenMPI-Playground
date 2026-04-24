# cannon-matrix-multiplication implementation

## input file format

    <number of rows> <number of columns>
    <data> <data> ... <data> # <number of columns> elements
    ...
    <data> <data> ... <data> # <number of rows> row elements

## benchmarking

Note: compiled with mpicc -o3 on Ubuntu

![Exeuction time average](plots/Picture1.png)
![Speedup](plots/Picture2.png)
![Efficiency](plots/Picture3.png)