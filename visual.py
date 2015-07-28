import matplotlib.pyplot as plt
from sklearn.metrics import roc_curve, auc
import numpy as np
from sklearn.tree import _tree


def plot_roc(fname,y_test,prob):
    # Compute ROC curve and area the curve
    fpr, tpr, thresholds = roc_curve(y_test, prob)
    roc_auc = auc(fpr, tpr)
    # Plot ROC curve
    plt.clf()
    plt.plot(fpr, tpr, label='ROC curve (area = %0.2f)' % roc_auc)
    plt.plot([0, 1], [0, 1], 'k--')
    plt.xlim([0.0, 1.0])
    plt.ylim([0.0, 1.0])
    plt.xlabel('False Positive Rate')
    plt.ylabel('True Positive Rate')
    plt.title('Receiver operating characteristic example')
    plt.legend(loc="lower right")
    plt.savefig(fname, bbox_inches=0)
    plt.show()

def plot_distance_matrix(dist,labels,fname=None):
    fig, ax = plt.subplots(nrows=1, ncols=1)
    fig.set_figheight(20);
    fig.set_figwidth(20);        

    plt.imshow(dist, interpolation='none',cmap='summer')

    N = dist.shape[1]
    plt.xlim((0,N))
    plt.ylim((0,N))
    ticks = range(0,N)
    ax.set_yticks(ticks)
    ax.set_yticklabels(labels)
    ax.set_xticks(ticks)
    ax.set_xticklabels(labels)
    plt.setp( ax.xaxis.get_majorticklabels(), rotation=-90 )    
    plt.colorbar()
    
    if(fname != None):
        plt.savefig(fname, bbox_inches=0) 
    else: 
        plt.show()           
    plt.close();

def plot_confidence(mat):
    x = range(0,len(mat))
    mu = np.mean(mat,1).tolist()
    dev =  np.std(mat,axis=1)
    
    
    x1,x2,y1,y2 = plt.axis()
    plt.fill(np.concatenate([x, x[::-1]]), \
            np.concatenate([mu - 1.9600 * dev,
                           (mu + 1.9600 * dev)[::-1]]), \
            alpha=.2, fc='b', ec='None', label='95% confidence interval')
    plt.errorbar(x,mu,dev,fmt='r.', markersize=10, label=u'Observations')
    plt.xlim(0,len(mat))
    plt.ylim(0.45,1)
    
def plot_dendrogram(dist,labels,fname=None):
    from scipy.cluster.hierarchy import linkage, dendrogram
    
    data_link = linkage(dist,method='complete') # computing the linkage    
    fig, ax = plt.subplots(nrows=1, ncols=1)
    den = dendrogram(data_link,labels=labels,orientation='left',show_leaf_counts=True,leaf_font_size=24)#X.dtype.names)
    fig.set_figheight(40);
    fig.set_figwidth(25);        
    idx1 = den['leaves']
    plt.setp( ax.xaxis.get_majorticklabels(), rotation=-90 )    
    plt.ylabel('Question')
    plt.suptitle('Answer Clustering', fontweight='bold', fontsize=14);
    if fname != None:    
        plt.savefig(fname, bbox_inches=0)
    else: 
        plt.show()           
        
    plt.close();
    return data_link


def export_dict(dectree, feature_names=None, max_depth=None):   
    tree_ = dectree.tree_

    # i is the element in the tree_ to create a dict for
    def recur(i, depth=0) :
        if max_depth is not None and depth > max_depth :
            return None
        if i == _tree.TREE_LEAF :
            return None

        feature = int(tree_.feature[i])
        threshold = float(tree_.threshold[i])
        if feature == _tree.TREE_UNDEFINED :
            feature = None
            threshold = None
            value = [map(int, l) for l in tree_.value[i].tolist()]
        else :
            value = None
            if feature_names :
                feature = feature_names[feature]

        return {
            'feature' : feature,
            'threshold' : threshold,
            'impurity' : float(tree_.impurity[i]),
            'n_node_samples' : int(tree_.n_node_samples[i]),
            'left'  : recur(tree_.children_left[i],  depth + 1),
            'right' : recur(tree_.children_right[i], depth + 1),
            'value' : value,
        }
    return recur(0)
    
#convert tree to the formatting used in the d3js tree visualizations
def to_d3js(tree,fname,labels,types,scaler=None):
    vizdict = export_dict(tree, labels)    

    def reverse_scaling(varname,treshold):
        if scaler != None:
            xi = [ treshold if lbl == varname else 0 for lbl in labels]
            idx = np.where(np.array(xi) != 0)[0][0]
            xi = scaler.inverse_transform(xi)
            return xi[idx]
        else:
            return treshold
    def printTreshold(varname,treshold,isleft):
        if types[varname] == "dummy":
            if isleft:
                return "No"
            else:
                return "Yes"
        elif types[varname] == "year":
            treshold = str(int(reverse_scaling(varname,treshold)))
            #treshold = str(pd.to_datetime(treshold))
            if isleft:
                return "Before "+treshold
            else:
                return "After "+treshold
        elif varname == ("officer_experience_time"):
            treshold = reverse_scaling(varname,treshold)
            treshold = str(int(treshold / (24*3600*10e8))) +" days"
        elif varname == ("officer_experience_loanrank"):
            treshold = reverse_scaling(varname,treshold)
            treshold = str(int(treshold)) +" loans"
        else:
            #treshold = str(int(treshold,0))   
            treshold = str(int(reverse_scaling(varname,treshold)))        
        if isleft:
            return "<="+treshold
        else:
            return ">"+treshold
    def printName(varname):
        nicename = varname
        if types[varname] == "dummy":
            splitidx = nicename.rfind('_')
            nicename = nicename[0:splitidx] + " = " +nicename[splitidx+1:]+" ?"
        return nicename
        
    def printnode(vd,level,linkrule):
        output=""
        indent = ""
        for i in range(0,level):
            indent += "\t"
        if(vd['feature'] is None):
            v = vd['value'][0]
            prob = np.round(v[1]*100.0 / (v[0]+v[1]),2)
            output += indent+'{"name":"LEAF",\n'
            output += indent+'  "probability":'+str(prob)+',\n'
            output += indent+' "rule":"'+linkrule+'",\n'        
            output += indent+'  "size":'+str(vd['n_node_samples'])+'}'
            return [output,v[1],v[0]]
        else:
            #children
            [leftstr,lc_pos,lc_neg]   = printnode(vd['left'],level+1,printTreshold(vd['feature'],vd['threshold'],1))
            [rightstr,rc_pos,rc_neg] = printnode(vd['right'],level+1,printTreshold(vd['feature'],vd['threshold'],0))   
            pos = lc_pos + rc_pos
            neg = lc_neg + rc_neg
            prob = np.round(pos*100.0 / (pos+neg),2)
            
            #print info
            output += indent+"{\n"
            output += indent+' "name":"'+printName(vd['feature'])+'",\n'
            output += indent+' "impurity":"'+str(vd['impurity'])+'",\n'
            output += indent+' "rule":"'+linkrule+'",\n'
            output += indent+' "size":"'+str(vd['n_node_samples'])+'",\n'            
            output += indent+' "probability":"'+str(prob)+'",\n'                        
            output += indent+' "children": [\n'
            output += indent+leftstr+",\n"
            output += indent+rightstr
            output += indent+"]}\n"
            return [output,pos,neg]
        
    [output ,_,_]= printnode(vizdict,0,"")
    with open(fname, 'w') as f:
        f.write(output)
	return output