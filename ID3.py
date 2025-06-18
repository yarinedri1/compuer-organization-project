import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import load_breast_cancer
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler

def entropy(y):
    p = np.mean(y)
    if p == 0 or p == 1:
        return 0
    return -p * np.log2(p) - (1 - p) * np.log2(1 - p)


def split_dataset(X, y, feature_index, value, is_continuous):
    if is_continuous[feature_index]:
        left_mask = X[:, feature_index] <= value
        right_mask = X[:, feature_index] > value
    else:
        left_mask = X[:, feature_index] == value
        right_mask = X[:, feature_index] != value
    return X[left_mask], y[left_mask], X[right_mask], y[right_mask]

def information_gain(X, y, feature_index, value, is_continuous):
    X_left, y_left, X_right, y_right = split_dataset(X, y, feature_index, value, is_continuous)

    if len(y_left) == 0 or len(y_right) == 0:
        return 0

    H_before = entropy(y)
    H_left = entropy(y_left)
    H_right = entropy(y_right)
    weight_left = len(y_left) / len(y)
    weight_right = len(y_right) / len(y)
    H_after = weight_left * H_left + weight_right * H_right    
    return H_before - H_after

def candidate_thresholds(values):
    return [
        np.mean(values),
        np.median(values),
        np.percentile(values, 25),
        np.percentile(values, 75)
    ]

def best_split(X, y, is_continuous):
    best_gain = 0
    best_feature = None
    best_value = None
    n_features = X.shape[1]
    for feature_index in range(n_features):
        values = X[:, feature_index]

        if is_continuous[feature_index]:
            thresholds = [
                np.mean(values),
                np.median(values),
                np.percentile(values, 25),
                np.percentile(values, 75)
            ]
        else:
            thresholds = np.unique(values)
        for val in thresholds:
            gain = information_gain(X, y, feature_index, val, is_continuous)
            if gain > best_gain:
                best_gain = gain
                best_feature = feature_index
                best_value = val
    return best_feature, best_value, best_gain

def is_pure(y):
    return len(np.unique(y)) == 1

def majority_class(y):
    return np.bincount(y).argmax()

def build_tree(X, y, is_continuous, depth=0, max_depth=None):
    if is_pure(y) or (max_depth is not None and depth >= max_depth):
        return majority_class(y)
    feature_index, value, gain = best_split(X, y, is_continuous)

    if gain == 0:
        return majority_class(y)

    X_left, y_left, X_right, y_right = split_dataset(X, y, feature_index, value, is_continuous)

    left_subtree = build_tree(X_left, y_left, is_continuous, depth + 1, max_depth)
    right_subtree = build_tree(X_right, y_right, is_continuous, depth + 1, max_depth)

    return {
        'feature_index': feature_index,
        'value': value,
        'is_continuous': is_continuous[feature_index],
        'left': left_subtree,
        'right': right_subtree
    }

def predict_sample(tree, sample):
    if not isinstance(tree, dict):
        return tree

    feature_index = tree['feature_index']
    value = tree['value']
    is_continuous = tree['is_continuous']

    if is_continuous:
        if sample[feature_index] <= value:
            return predict_sample(tree['left'], sample)
        else:
            return predict_sample(tree['right'], sample)
    else:
        if sample[feature_index] == value:
            return predict_sample(tree['left'], sample)
        else:
            return predict_sample(tree['right'], sample)

def predict(tree, X):
    return np.array([predict_sample(tree, sample) for sample in X])

def accuracy(y_true, y_pred):
    return np.mean(y_true == y_pred)


def compute_tree_width(tree):
    """Compute total number of leaf nodes (tree width)."""
    if not isinstance(tree, dict):
        return 1
    return compute_tree_width(tree['left']) + compute_tree_width(tree['right'])

def plot_decision_tree(tree, feature_names, X, y, title="Decision Tree"):
    fig, ax = plt.subplots(figsize=(24, 12))
    ax.axis("off")
    total_width = compute_tree_width(tree)
    _plot_tree_recursive(ax, tree, feature_names, X, y, x=0.5, y_pos=1.0,
                         total_width=total_width, dx=1.0 / total_width, dy=0.12)
    ax.set_title(title, fontsize=18)
    plt.tight_layout()
    plt.show()

def _plot_tree_recursive(ax, tree, feature_names, X, y, x, y_pos, total_width, dx, dy, depth=0):
    if not isinstance(tree, dict):
        counts = np.bincount(y, minlength=2)
        total = counts.sum()
        label = f"Predict: {tree}\n[0: {counts[0]}, 1: {counts[1]}]"
        ax.text(x, y_pos, label, ha="center", va="center",
                bbox=dict(boxstyle="round,pad=0.5", fc="lightgreen", ec="black"),
                fontsize=10)
        return

    feature_name = feature_names[tree['feature_index']]
    value = tree['value']
    is_cont = tree['is_continuous']

    node_label = f"{feature_name} <= {value:.2f}" if is_cont else f"{feature_name} == {value}"
    ax.text(x, y_pos, node_label, ha="center", va="center",
            bbox=dict(boxstyle="round,pad=0.5", fc="lightblue", ec="black"),
            fontsize=10)

    # Split the data for the current node
    X_left, y_left, X_right, y_right = split_dataset(
        X, y, tree['feature_index'], tree['value'], [tree['is_continuous']] * X.shape[1])

    # Compute subtree widths
    width_left = compute_tree_width(tree['left'])
    width_right = compute_tree_width(tree['right'])
    total = width_left + width_right

    # Child positions
    x_left = x - dx * (width_right / total)
    x_right = x + dx * (width_left / total)
    y_child = y_pos - dy

    # Draw edges
    ax.plot([x, x_left], [y_pos, y_child], "k-")
    ax.plot([x, x_right], [y_pos, y_child], "k-")

    # Recurse
    _plot_tree_recursive(ax, tree['left'], feature_names, X_left, y_left,
                         x_left, y_child, total_width, dx, dy, depth + 1)
    _plot_tree_recursive(ax, tree['right'], feature_names, X_right, y_right,
                         x_right, y_child, total_width, dx, dy, depth + 1)



def main():
    data = load_breast_cancer()
    X = data.data
    y = data.target

    is_continuous = [np.issubdtype(X[:, i].dtype, np.number) for i in range(X.shape[1])]
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    tree = build_tree(X_train, y_train, is_continuous, max_depth=4)  # try smaller depth for easier viewing

    y_pred_train = predict(tree, X_train)
    y_pred_test = predict(tree, X_test)

    print(f"Train Accuracy: {accuracy(y_train, y_pred_train):.2f}")
    print(f"Test Accuracy: {accuracy(y_test, y_pred_test):.2f}")

    print("\nDecision Tree Structure:\n")
    plot_decision_tree(tree, data.feature_names, X_train, y_train)

   
if __name__ == "__main__":
    main()
