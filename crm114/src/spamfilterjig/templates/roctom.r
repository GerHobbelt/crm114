roctom<-function (truth, data, rule, seqlen = length(truth),
    cutpts = seq(min(data), max(data), length = seqlen), markerLabel = "unnamed marker",
    caseLabel = "unnamed diagnosis")
{
    if (!all(sort(unique(truth)) == c(0, 1)))
        stop("'truth' variable must take values 0 or 1")
    np <- length(cutpts)
    sens <- rep(NA, np)
    spec <- rep(NA, np)
    pred <- rep(NA, np)
    o<-order(data)
    d<-data[o]	
    t<-truth[o]
    s=sum(t)
    h=np-sum(t)		
    fp=h
    fn=0
    for (i in 1:np) {
        if (t[i]==0){
	  fp=fp-1
        }
        if (t[i]==1){
	  fn=fn+1 
        }
        sens[i] = 1-fn/s
        spec[i] = 1-fp/h
    }
    new("rocc", spec = spec, sens = sens, rule = rule, cuts = cutpts,markerLabel = markerLabel, caseLabel = caseLabel)
}

library(ROC)

x = read.table("tmp/NAME.all.spss0", header=TRUE)
z = roctom(x$qrel - 1,x$score,dxrule.sca)
y = AUC(z)
y
