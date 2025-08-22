#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*** --------------------------------------------------***/

/*** --------------------------------------------------***/


// 參數樹節點結構（與之前相同）
typedef struct ParamNode {
    char* key;
    char* value;
    struct ParamNode** children;
    int child_count;
} ParamNode;

// 創建節點（與之前相同）
ParamNode* create_node(const char* key, const char* value) {
    ParamNode* node = (ParamNode*)malloc(sizeof(ParamNode));
    if (!node) {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    node->key = strdup(key);
    node->value = value ? strdup(value) : NULL;
    node->children = NULL;
    node->child_count = 0;
    return node;
}

// 添加子節點（與之前相同）
void add_child(ParamNode* parent, ParamNode* child) {
    parent->children = (ParamNode**)realloc(parent->children, 
                                          (parent->child_count + 1) * sizeof(ParamNode*));
    if (!parent->children) {
        printf("Memory reallocation failed!\n");
        exit(1);
    }
    parent->children[parent->child_count] = child;
    parent->child_count++;
}

// 列印樹（與之前相同）
void print_tree(ParamNode* node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s: %s\n", node->key, node->value ? node->value : "(no value)");
    for (int i = 0; i < node->child_count; i++) {
        print_tree(node->children[i], depth + 1);
    }
}

// 釋放樹記憶體（與之前相同）
void free_tree(ParamNode* node) {
    if (!node) return;
    free(node->key);
    if (node->value) free(node->value);
    for (int i = 0; i < node->child_count; i++) {
        free_tree(node->children[i]);
    }
    free(node->children);
    free(node);
}

// 比較兩個樹的 diff 函數
void tree_diff(ParamNode* tree1, ParamNode* tree2, int depth) {
    if (!tree1 && !tree2) return; // 兩個節點都為空，無差異
    if (!tree1 || !tree2) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("Difference: %s node missing\n", tree1 ? "tree2" : "tree1");
        return;
    }

    // 比較鍵
    if (strcmp(tree1->key, tree2->key) != 0) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("Difference: key mismatch (%s vs %s)\n", tree1->key, tree2->key);
    }

    // 比較值
    if ((tree1->value && !tree2->value) || (!tree1->value && tree2->value) ||
        (tree1->value && tree2->value && strcmp(tree1->value, tree2->value) != 0)) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("Difference: value mismatch at %s (%s vs %s)\n", 
               tree1->key, 
               tree1->value ? tree1->value : "(null)", 
               tree2->value ? tree2->value : "(null)");
    }

    // 比較子節點數量
    if (tree1->child_count != tree2->child_count) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("Difference: child count mismatch at %s (%d vs %d)\n", 
               tree1->key, tree1->child_count, tree2->child_count);
    }

    // 簡單假設子節點按順序匹配（實際應用中可能需要更複雜的匹配算法）
    int max_children = tree1->child_count > tree2->child_count ? tree1->child_count : tree2->child_count;
    for (int i = 0; i < max_children; i++) {
        ParamNode* child1 = (i < tree1->child_count) ? tree1->children[i] : NULL;
        ParamNode* child2 = (i < tree2->child_count) ? tree2->children[i] : NULL;
        tree_diff(child1, child2, depth + 1);
    }
}


int main(int argc, char *argv[]) 
{
    // 創建第一棵樹
    ParamNode* tree1 = create_node("network", NULL);
    ParamNode* t1_input_size = create_node("input_size", "784");
    ParamNode* t1_layers = create_node("layers", NULL);
    ParamNode* t1_learning_rate = create_node("learning_rate", "0.001");
    add_child(tree1, t1_input_size);
    add_child(tree1, t1_layers);
    add_child(tree1, t1_learning_rate);
    ParamNode* t1_layer1 = create_node("layer1", NULL);
    add_child(t1_layers, t1_layer1);
    ParamNode* t1_l1_units = create_node("units", "128");
    ParamNode* t1_l1_activation = create_node("activation", "relu");
    add_child(t1_layer1, t1_l1_units);
    add_child(t1_layer1, t1_l1_activation);

    // 創建第二棵樹（有差異）
    ParamNode* tree2 = create_node("network", NULL);
    ParamNode* t2_input_size = create_node("input_size", "784");
    ParamNode* t2_layers = create_node("layers", NULL);
    ParamNode* t2_learning_rate = create_node("learning_rate", "0.002"); // 不同值
    add_child(tree2, t2_input_size);
    add_child(tree2, t2_layers);
    add_child(tree2, t2_learning_rate);
    ParamNode* t2_layer1 = create_node("layer1", NULL);
    add_child(t2_layers, t2_layer1);
    ParamNode* t2_l1_units = create_node("units", "256"); // 不同值
    add_child(t2_layer1, t2_l1_units); // 缺少 activation 節點

    // 列印兩棵樹
    printf("Tree 1:\n");
    print_tree(tree1, 0);
    printf("\nTree 2:\n");
    print_tree(tree2, 0);

    // 執行 diff
    printf("\nDifferences:\n");
    tree_diff(tree1, tree2, 0);

    // 清理記憶體
    free_tree(tree1);
    free_tree(tree2);
    return 0;
	return 0;
}
