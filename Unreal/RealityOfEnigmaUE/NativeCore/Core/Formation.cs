namespace Vaelorn;

public static class Formation
{
    public static int[] Normalize(IEnumerable<int>? saved,int count)
    {
        if(count<0||count>9)throw new ArgumentOutOfRangeException(nameof(count));
        var source=saved?.ToArray()??Array.Empty<int>();var used=new HashSet<int>();var result=new int[count];
        for(int i=0;i<count;i++){
            int cell=i<source.Length?source[i]:-1;
            if(cell<0||cell>8||used.Contains(cell))cell=Enumerable.Range(0,9).First(n=>!used.Contains(n));
            result[i]=cell;used.Add(cell);
        }
        return result;
    }
}
