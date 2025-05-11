#include "common.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    print_hello();

    // JSON 데이터 조작 예제
    // 샘플 JSON 생성
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", "Alice");
    cJSON_AddNumberToObject(root, "score", 100);

    // 값 수정
    cJSON_ReplaceItemInObject(root, "name", cJSON_CreateString("Bob"));
    cJSON_ReplaceItemInObject(root, "score", cJSON_CreateNumber(200));

    // JSON 문자열로 출력
    char *json_str = cJSON_Print(root);
    printf("%s\n", json_str);

    // 메모리 해제
    cJSON_Delete(root);
    free(json_str);

    return 0;
} 