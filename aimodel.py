import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import classification_report, accuracy_score

# ---------------- LOAD DATA ----------------
df = pd.read_csv("dataset.csv")

# Features (inputs)
X = df[["mq2", "mq7"]]

# Labels (output)
y = df["label"]

# ---------------- ENCODE LABELS ----------------
le = LabelEncoder()
y_encoded = le.fit_transform(y)

print("Label Mapping:")
for i, label in enumerate(le.classes_):
    print(f"{label} → {i}")

# ---------------- TRAIN TEST SPLIT ----------------
X_train, X_test, y_train, y_test = train_test_split(
    X, y_encoded, test_size=0.2, random_state=42
)

# ---------------- TRAIN MODEL ----------------
model = LogisticRegression()
model.fit(X_train, y_train)

# ---------------- EVALUATE ----------------
y_pred = model.predict(X_test)

print("\nAccuracy:", accuracy_score(y_test, y_pred))
print("\nClassification Report:")
print(classification_report(y_test, y_pred, target_names=le.classes_))

# ---------------- SHOW MODEL PARAMETERS ----------------
print("\nModel Coefficients:")
print("Weights:", model.coef_)
print("Bias:", model.intercept_)