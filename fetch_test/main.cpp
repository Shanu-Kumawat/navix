#include <Eigen/Dense>
#include <iostream>
int main() { Eigen::MatrixXd m(2,2); m(0,0) = 3; std::cout << m(0,0) << std::endl; return 0; }
