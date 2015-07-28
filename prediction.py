import sklearn
import numpy as np

#train a logistic regression model
def train_logistic(X,y):
    from sklearn.svm import SVC
    from sklearn.grid_search import GridSearchCV
    from sklearn.linear_model import LogisticRegression
    
    clf = LogisticRegression()

    Crange = np.logspace(-8, 1, 8)
    grid = GridSearchCV(clf, param_grid={'C': Crange},scoring='roc_auc', cv=5)
    grid.fit(X, y)
    
    clf.C = grid.best_params_['C']
    clf.fit(X,y)
    return clf
    
def predictive_performanceXY(X,y):
    from sklearn.cross_validation import train_test_split      
    from sklearn.metrics import roc_curve, auc

    #format & split data
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.20)    

    #build model
    clf = train_logistic(X_train,y_train)
    prob = clf.predict_proba(X_test)

    #evaluate model
    proba = clf.predict_proba(X_test)    
    fpr, tpr, thresholds = roc_curve(y_test, prob[:, 1])
    roc_auc = auc(fpr, tpr)

    return roc_auc

def predictive_performance(X_train,X_test,y_train,y_test):
    from sklearn.metrics import roc_curve, auc

    #build model
    clf = train_logistic(X_train,y_train)
    prob = clf.predict_proba(X_test)

    #evaluate model
    fpr, tpr, thresholds = roc_curve(y_test, prob[:, 1])
    roc_auc = auc(fpr, tpr)

    return roc_auc
