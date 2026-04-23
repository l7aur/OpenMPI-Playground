#include <iostream>
#include <fstream>
#include <vector>

using matrix = std::vector<std::vector<int>>;

matrix readMatrix(std::ifstream& fin) {
    int n, m;
    fin >> n >> m;

    matrix mat(n);
    for (int i = 0; i < n; i++) {
        mat[i] = std::vector<int>(m);
        for (int j = 0; j < m; j++)
            fin >> mat[i][j];
    }

    return mat;
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage ./matrix_multiply <path_to_matrix> <path_to_matrix>\n";
        return 1;
    }
    std::ifstream fin(argv[1]);
    matrix m1 = readMatrix(fin);
    fin.close();

    fin.open(argv[2]);
    matrix m2 = readMatrix(fin);
    fin.close();

    matrix r = matrix(m1.size());
    for (int i = 0; i < r.size(); i++)
        r[i] = std::vector<int>(m2[i].size(), 0);

    if (m1[0].size() != m2.size()) {
        std::cerr << "Matrices are not compatible for multiplication!\n";
        return 1;
    }

    for (int i = 0; i < m1.size(); i++)
        for (int j = 0; j < m2[0].size(); j++) 
            for (int k = 0; k < m1[0].size(); k++)
                r[i][j] += m1[i][k] * m2[k][j];

    for (int i = 0; i < r.size(); i++, std::cout << "\n")
        for(int j = 0; j < r[i].size(); j++)
            std::cout << r[i][j] << " ";

    return 0;
}