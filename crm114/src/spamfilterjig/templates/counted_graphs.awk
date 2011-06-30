#
# add ls #no to each plot line:
#

BEGIN   {
	linestyle += 0;
	if (linestyle == 0)
	{
    		linestyle = 1;
	}
	# plot = 1;
}

END	{
	if (plot > 2)
	{
		printf("\n\n\n\n\n");
	}
}

/^plot/	{
	printf("%s\n", $0);
	   linestyle = 1;
	plot = 2;
	next;
}

/ with .*\\$/   {
	if (plot)
	{
	        sub(/\\$/, "", $0);
	        sub(/^, /, "", $0);
	        gsub(/ ls [0-9]+ /, "", $0);
        	printf(", %s ls %d \\\n", $0, linestyle++);
        	next;
	}
}


    {
    printf("%s\n", $0);
}

    
        
