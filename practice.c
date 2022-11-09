int main(){
  char a[10];
  parse(a);	

  printf("parse의 결괏값을 받은 a : %s가 왔다. \n", a);
}

int parse(char *a){
  char* b = "hi";
  a = b;
  printf("parse내에서 찍은 a : %s가 왔다. \n", a);
}
