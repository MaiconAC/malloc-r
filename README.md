# Malloc recreation

- quando um programa é executado, o kernel cria um espaco de enderecamento virtual
composto por stack (variaveis locais), libs, heap (alocacoes dinamicas), globais e
codigo executavel, o malloc gerencia o heap

- o malloc aloca os objetos dentro da memoria, o kernel nao sabe de nada, so que tal
processo alocou

- para funcionar o malloc, ele primeiro pede memoria para o kernel e depois a divide
internamente para uso

- a reserva de memoria pode ser feita com duas syscalls, brk() e mmap(), brk move o
program break para cima e expande o heap, bom para programas convencionais e nao
diminui de tamanho facilmente; já o mmap faz um mapeamento de memória que pode ser
devolvido ao sistema operacional a qualquer momento com munmap(), muito util para
alocacoes grandes

- usando sbrk(0) é possível descobrir qual o program break atual, e se tentar adicionar
valor no endereço seguinte usando um ponteiro, vai dar erro de segfault, exemplo:

    void *programBrkAddress = sbrk(0);

    int *testPtr = (int *) programBrkAddress + 1;
    *testPtr = 10;

- sempre colocar um brk(initial) no final para voltar o program break para o começo
e não dar problema de memoria

- quando é aumentado o break, mesmo que seja selecionado o tamanho de 2 ints (exemplo),
o kernel vai aumentar o tamanho de uma página de memória de normalmente 4KB. Apesar de
não dar erro de segfault nesse caso, a memória ainda pode ser corrompida, e esse tipo
de erro só ferramentas como valgrind detectam

	brk(initialBrkAddress + 2);

	int *testPtr = (int *) initialBrkAddress;
	*testPtr = 10;
	*(testPtr + 1) = 20;
	*(testPtr + 2) = 30;
	*(testPtr + 3) = 40;
