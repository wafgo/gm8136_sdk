void bar(void)
{
    int *ptr = 0;
    int tmp = 0;

    tmp = *ptr; 
}

void foo(void)
{
    bar();
}

int main(void)
{
    foo();

    return 0;
}
