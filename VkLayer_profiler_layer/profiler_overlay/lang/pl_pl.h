// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#pragma once
#include "en_us.h"

namespace Profiler
{
    struct DeviceProfilerOverlayLanguage_PL : DeviceProfilerOverlayLanguage_Base
    {
        inline static constexpr char Language[] = u8"Polski";

        inline static constexpr char Device[] = u8"Urządzenie";
        inline static constexpr char Instance[] = u8"Instancja";
        inline static constexpr char Pause[] = u8"Wstrzymaj";
        inline static constexpr char Save[] = u8"Zapisz";
        inline static constexpr char Load[] = u8"Wczytaj";
        inline static constexpr char SaveTrace[] = u8"Zapisz dane";

        // Menu
        inline static constexpr char FileMenu[] = u8"Plik" PROFILER_MENU;
        inline static constexpr char WindowMenu[] = u8"Okno" PROFILER_MENU;
        inline static constexpr char PerformanceMenuItem[] = u8"Wydajność" PROFILER_MENU_ITEM;
        inline static constexpr char PerformanceCountersMenuItem[] = u8"Liczniki wydajności" PROFILER_MENU_ITEM;
        inline static constexpr char QueueUtilizationMenuItem[] = u8"Wykorzystanie kolejek" PROFILER_MENU_ITEM;
        inline static constexpr char TopPipelinesMenuItem[] = u8"Najdłuższe stany potoku" PROFILER_MENU_ITEM;
        inline static constexpr char MemoryMenuItem[] = u8"Pamięć" PROFILER_MENU_ITEM;
        inline static constexpr char InspectorMenuItem[] = u8"Inspektor" PROFILER_MENU_ITEM;
        inline static constexpr char StatisticsMenuItem[] = u8"Statystyki" PROFILER_MENU_ITEM;
        inline static constexpr char SettingsMenuItem[] = u8"Ustawienia" PROFILER_MENU_ITEM;
        inline static constexpr char ApplicationInfoMenuItem[] = u8"Informacje o aplikacji" PROFILER_MENU_ITEM;

        // Tabs
        inline static constexpr char Performance[] = u8"Wydajność###Performance";
        inline static constexpr char Memory[] = u8"Pamięć###Memory";
        inline static constexpr char Inspector[] = u8"Inspektor###Inspector";
        inline static constexpr char Statistics[] = u8"Statystyki###Statistics";
        inline static constexpr char Settings[] = u8"Ustawienia###Settings";
        inline static constexpr char ApplicationInfo[] = "Informacje o aplikacji###ApplicationInfo";

        // Performance tab
        inline static constexpr char GPUTime[] = u8"Czas GPU";
        inline static constexpr char CPUTime[] = u8"Czas CPU";
        inline static constexpr char RenderPasses[] = u8"Render passy";
        inline static constexpr char Pipelines[] = u8"Stany potoku";
        inline static constexpr char Drawcalls[] = u8"Komendy";
        inline static constexpr char Constant[] = u8"Stała";
        inline static constexpr char Duration[] = u8"Czas trwania";
        inline static constexpr char Height[] = u8"Wysokość";
        inline static constexpr char ShowIdle[] = u8"Pokaż przerwy";
        inline static constexpr char HistogramGroups[] = u8"Grupowanie histogramu";
        inline static constexpr char GPUCycles[] = u8"Cykle GPU";
        inline static constexpr char QueueUtilization[] = u8"Wykorzystanie kolejek###QueueUtilization";
        inline static constexpr char TopPipelines[] = u8"Najdłuższe stany potoku###Top pipelines";
        inline static constexpr char PerformanceCounters[] = u8"Liczniki wydajności###Performance counters";
        inline static constexpr char Metric[] = u8"Metryka";
        inline static constexpr char Contribution[] = u8"Udział";
        inline static constexpr char Frame[] = u8"Ramka";
        inline static constexpr char Pipeline[] = u8"Stan potoku";
        inline static constexpr char Stages[] = u8"Shadery";
        inline static constexpr char SetRef[] = u8"Ustaw ref";
        inline static constexpr char Value[] = u8"Wartość";
        inline static constexpr char FrameBrowser[] = u8"Przeglądarka ramki###Frame browser";
        inline static constexpr char SubmissionOrder[] = u8"Kolejność wykonania";
        inline static constexpr char DurationDescending[] = u8"Malejący czas wykonania";
        inline static constexpr char DurationAscending[] = u8"Rosnący czas wykonania";
        inline static constexpr char Sort[] = u8"Sortowanie";
        inline static constexpr char CustomStatistics[] = u8"Własne statystyki";
        inline static constexpr char Container[] = u8"Kontener";
        inline static constexpr char Inspect[] = u8"Inspekcja";
        inline static constexpr char ShowPerformanceMetrics[] = u8"Pokaż liczniki wydajności";
        inline static constexpr char CopyToClipboard[] = u8"Kopiuj do schowka";
        inline static constexpr char CopyName[] = u8"Kopiuj nazwę";
        inline static constexpr char ShowMore[] = u8"Pokaż więcej...";
        inline static constexpr char ShowLess[] = u8"Pokaż mniej";

        inline static constexpr char ShaderCapabilityTooltipFmt[] = u8"Co najmniej jeden shader w potoku korzysta z funkcjonalności '%s'.";
        inline static constexpr char ShaderObjectsTooltip[] = u8"Potok utworzony z objektów VkShaderEXT za pomocą vkCmdBindShadersEXT.";

        inline static constexpr char PerformanceCountersFilter[] = u8"Filtr";
        inline static constexpr char PerformanceCountersRange[] = u8"Zakres";
        inline static constexpr char PerformanceCountersSet[] = u8"Zbiór metryk";
        inline static constexpr char PerformanceCountesrNotAvailable[] = u8"Liczniki wydajności nie są dostępne.";
        inline static constexpr char PerformanceCountersNotAvailableForCommandBuffer[] = u8"Liczniki wydajności nie są dostępne dla wybranego bufora komend.";

        // Memory tab
        inline static constexpr char MemoryHeapUsage[] = u8"Wykorzystanie stert pamięci";
        inline static constexpr char MemoryHeap[] = u8"Sterta";
        inline static constexpr char Allocations[] = u8"Alokacje";
        inline static constexpr char MemoryTypeIndex[] = u8"Indeks typu pamięci";

        // Statistics tab
        inline static constexpr char ShowEmptyStatistics[] = u8"Pokaż puste statystyki...";
        inline static constexpr char HideEmptyStatistics[] = u8"Ukryj puste statystyki";
        inline static constexpr char StatName[] = u8"Nazwa";
        inline static constexpr char StatCount[] = u8"Liczba";
        inline static constexpr char StatTotal[] = u8"Razem";
        inline static constexpr char StatMin[] = u8"Min";
        inline static constexpr char StatMax[] = u8"Max";
        inline static constexpr char StatAvg[] = u8"Śr.";
        inline static constexpr char DrawCalls[] = u8"Komendy rysujące";
        inline static constexpr char DrawCallsIndirect[] = u8"Komendy rysujące (typu indirect)";
        inline static constexpr char DrawMeshTasksCalls[] = u8"Komendy rysujące mesh";
        inline static constexpr char DrawMeshTasksIndirectCalls[] = u8"Komendy rysujące mesh (typu indirect)";
        inline static constexpr char DispatchCalls[] = u8"Komendy obliczeniowe";
        inline static constexpr char DispatchCallsIndirect[] = u8"Komendy obliczeniowe (typu indirect)";
        inline static constexpr char TraceRaysCalls[] = u8"Komendy śledzenia promieni";
        inline static constexpr char TraceRaysIndirectCalls[] = u8"Komendy śledzenia promieni (typu indirect)";
        inline static constexpr char CopyBufferCalls[] = u8"Kopie pomiędzy buforami";
        inline static constexpr char CopyBufferToImageCalls[] = u8"Kopie z buforów do obrazów";
        inline static constexpr char CopyImageCalls[] = u8"Kopie pomiędzy obrazami";
        inline static constexpr char CopyImageToBufferCalls[] = u8"Kopie z obrazów do buforów";
        inline static constexpr char PipelineBarriers[] = u8"Bariery";
        inline static constexpr char ColorClearCalls[] = u8"Czyszczenie obrazów kolorowych";
        inline static constexpr char DepthStencilClearCalls[] = u8"Czyszczenie obrazów głębii i bitmap";
        inline static constexpr char ResolveCalls[] = u8"Rozwiązania obrazów o wielu próbkach (MSAA)";
        inline static constexpr char BlitCalls[] = u8"Kopie pomiędzy obrazami ze skalowaniem";
        inline static constexpr char FillBufferCalls[] = u8"Wypełnienia buforów";
        inline static constexpr char UpdateBufferCalls[] = u8"Aktualizacje buforów";

        // Inspector tab
        inline static constexpr char PipelineState[] = u8"Stan potoku";
        inline static constexpr char PipelineStateNotAvailable[] = u8"Informacje o stanie potoku nie są dostępne.";

        // Settings tab
        inline static constexpr char Present[] = u8"Co ramkę";
        inline static constexpr char Submit[] = u8"Co przesłanie komend";
        inline static constexpr char SamplingMode[] = u8"Częstotliwość próbkowania";
        inline static constexpr char SyncMode[] = u8"Częstotliwość synchronizacji";
        inline static constexpr char InterfaceScale[] = u8"Skala interfejsu";
        inline static constexpr char ShowDebugLabels[] = u8"Pokaż etykiety";
        inline static constexpr char ShowShaderCapabilities[] = u8"Pokaż funkcjonalności shadera";
        inline static constexpr char TimeUnit[] = u8"Jednostka czasu";

        // Application info window
        inline static constexpr char VulkanVersion[] = u8"Wersja Vulkan";
        inline static constexpr char ApplicationName[] = u8"Nazwa aplikacji";
        inline static constexpr char ApplicationVersion[] = u8"Wersja aplikacji";
        inline static constexpr char EngineName[] = u8"Nazwa silnika";
        inline static constexpr char EngineVersion[] = u8"Wersja silnika";

        inline static constexpr char Unknown[] = u8"Nieznany";
    };
}
