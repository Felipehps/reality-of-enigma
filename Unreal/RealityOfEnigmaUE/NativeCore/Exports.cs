using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json.Nodes;
using Vaelorn;

public static unsafe class Exports
{
    [UnmanagedCallersOnly(EntryPoint="EnigmaOpen",CallConvs=new[]{typeof(CallConvCdecl)})]
    public static nint Open()=>GCHandle.ToIntPtr(GCHandle.Alloc(new GameSession()));
    [UnmanagedCallersOnly(EntryPoint="EnigmaClose",CallConvs=new[]{typeof(CallConvCdecl)})]
    public static void Close(nint handle){if(handle!=0)GCHandle.FromIntPtr(handle).Free();}
    [UnmanagedCallersOnly(EntryPoint="EnigmaFree",CallConvs=new[]{typeof(CallConvCdecl)})]
    public static void Free(nint result)=>Marshal.FreeCoTaskMem(result);
    [UnmanagedCallersOnly(EntryPoint="EnigmaExecute",CallConvs=new[]{typeof(CallConvCdecl)})]
    public static nint Execute(nint handle,byte* request)
    {
        string result;
        try {
            var session=(GameSession)GCHandle.FromIntPtr(handle).Target!;
            var input=JsonNode.Parse(Marshal.PtrToStringUTF8((nint)request)!)!.AsObject();
            result=session.Execute(input).ToJsonString();
        } catch(Exception ex) { result=new JsonObject { ["ok"]=false,["error"]=ex.Message }.ToJsonString(); }
        return Marshal.StringToCoTaskMemUTF8(result);
    }
}
