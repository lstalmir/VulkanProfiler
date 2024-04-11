// Copyright (c) 2024 Lukasz Stalmirski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "lang.h"

namespace Profiler
{
    inline const Lang& GetLanguage_pl_pl()
    {
        static Lang pl_pl;
        if( !pl_pl.Language )
        {
            pl_pl.LanguageName = u8"Polski";

            pl_pl.Device = u8"Urządzenie";
            pl_pl.Instance = u8"Instancja";
            pl_pl.Pause = u8"Wstrzymaj";
            pl_pl.Save = u8"Zapisz dane";

            pl_pl.Performance = u8"Wydajność";
            pl_pl.Memory = u8"Pamięć";
            pl_pl.Statistics = u8"Statystyki";
            pl_pl.Settings = u8"Ustawienia";

            pl_pl.GPUTime = u8"Czas GPU";
            pl_pl.CPUTime = u8"Czas CPU";
            pl_pl.RenderPasses = u8"Render passy";
            pl_pl.Pipelines = u8"Stany potoku";
            pl_pl.Drawcalls = u8"Komendy";
            pl_pl.HistogramGroups = u8"Grupowanie histogramu";
            pl_pl.GPUCycles = u8"Cykle GPU";
            pl_pl.TopPipelines = u8"Najdłuższe stany potoku";
            pl_pl.PerformanceCounters = u8"Liczniki wydajności";
            pl_pl.Metric = u8"Metryka";
            pl_pl.Frame = u8"Ramka";
            pl_pl.FrameBrowser = u8"Przeglądarka ramki";
            pl_pl.SubmissionOrder = u8"Kolejność wykonania";
            pl_pl.DurationDescending = u8"Malejący czas wykonania";
            pl_pl.DurationAscending = u8"Rosnący czas wykonania";
            pl_pl.Sort = u8"Sortowanie";
            pl_pl.CustomStatistics = u8"Własne statystyki";
            pl_pl.Container = u8"Kontener";

            pl_pl.ShaderCapabilityTooltipFmt = u8"Co najmniej jeden shader w potoku korzysta z funkcjonalności '%s'.";

            pl_pl.MemoryHeapUsage = u8"Wykorzystanie stert pamięci";
            pl_pl.MemoryHeap = u8"Sterta";
            pl_pl.Allocations = u8"Alokacje";
            pl_pl.MemoryTypeIndex = u8"Indeks typu pamięci";

            pl_pl.DrawCalls = u8"Komendy rysujące";
            pl_pl.DrawCallsIndirect = u8"Komendy rysujące (typu indirect)";
            pl_pl.DispatchCalls = u8"Komendy obliczeniowe";
            pl_pl.DispatchCallsIndirect = u8"Komendy obliczeniowe (typu indirect)";
            pl_pl.TraceRaysCalls = u8"Komendy śledzenia promieni";
            pl_pl.TraceRaysIndirectCalls = u8"Komendy śledzenia promieni (typu indirect)";
            pl_pl.CopyBufferCalls = u8"Kopie pomiędzy buforami";
            pl_pl.CopyBufferToImageCalls = u8"Kopie z buforów do obrazów";
            pl_pl.CopyImageCalls = u8"Kopie pomiędzy obrazami";
            pl_pl.CopyImageToBufferCalls = u8"Kopie z obrazów do buforów";
            pl_pl.PipelineBarriers = u8"Bariery";
            pl_pl.ColorClearCalls = u8"Czyszczenie obrazów kolorowych";
            pl_pl.DepthStencilClearCalls = u8"Czyszczenie obrazów głębii i bitmap";
            pl_pl.ResolveCalls = u8"Rozwiązania obrazów o wielu próbkach (MSAA)";
            pl_pl.BlitCalls = u8"Kopie pomiędzy obrazami ze skalowaniem";
            pl_pl.FillBufferCalls = u8"Wypełnienia buforów";
            pl_pl.UpdateBufferCalls = u8"Aktualizacje buforów";

            pl_pl.Language = u8"Język";
            pl_pl.Present = u8"Co ramkę";
            pl_pl.Submit = u8"Co przesłanie komend";
            pl_pl.SyncMode = u8"Moment synchronizacji";
            pl_pl.ShowDebugLabels = u8"Pokaż etykiety";
            pl_pl.ShowShaderCapabilities = u8"Pokaż funkcjonalności shadera";
            pl_pl.TimeUnit = u8"Jednostka czasu";

            pl_pl.Unknown = u8"Nieznany";
        }
        return pl_pl;
    }
}
