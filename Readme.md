
## Introduction

The project is the experimental code for the paper titled "BLAS: A Hybrid-Storage Blockchain Based Efficient Verifiable Log Audit System". In the BLAS, we propose the Index-Object Merkle Forest (IOMF), a novel authenticated data structure supporting efficient verifiable Boolean and range queries. The IOMF integrates an Object Merkle Tree (OMT) for data integrity, an Inverted Index Merkle Tree (IIMT) for Boolean queries, and Range Index Merkle B-Trees (RIMBT) for range queries, enhanced by bitmap indices for logical operations. We further develop the Authenticated Layered Index Structure(ALIS) based on IOMF for a hybrid-storage blockchain-based log auditing system (BLAS), ensuring fine-grained verifiable queries.Our system optimizes performance by segmenting block-level indices and aggregating indices to reduce maintenance and query costs.

## Necessary libraries or tools
1. gcc version 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04) 
2. GNU Make 4.3
3. [crypto++](https://github.com/weidai11/cryptopp)
4. [boost](https://www.boost.org/)
 
## Usage
1. Download the code to your directory.
2. Download [data.zip](https://zenodo.org/records/13163807/files/data.zip?download=1)、[db.zip](https://zenodo.org/records/13163807/files/db.zip?download=1) and [else.zip](https://zenodo.org/records/13163807/files/else.zip?download=1). In the root directory, unzip the file and name it according to the compressed package name.
3. Compile [crypto++](https://github.com/weidai11/cryptopp) and [boost](https://www.boost.org/). And you maybe need to modify Makefile to adapt your library file path.
4. Excute the code `make clean && make` and will generate executable file `main` in your directory.

## Contributing

We love your input! We want to make contributing to this project as easy and transparent as possible, whether it's:

- Reporting a bug
- Discussing the current state of the code
- Proposing new features

## License
By contributing, you agree that your contributions will be licensed under its MIT License.

## Contact 

