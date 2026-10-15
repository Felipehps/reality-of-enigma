using System.Text.Json.Serialization;
namespace Vaelorn;
[JsonSourceGenerationOptions(PropertyNameCaseInsensitive=true,WriteIndented=true)]
[JsonSerializable(typeof(Catalog))]
[JsonSerializable(typeof(SaveData))]
internal partial class GameJsonContext:JsonSerializerContext {}
