<script setup lang="ts">
import { computed, onMounted, ref, watch } from "vue";
import { barChart } from "svgplot-wasm";
import { renderers, timings, type Comparison, type Phase } from "../../benchmarks/timings";

const props = defineProps<{ comparison: Comparison; phase: Phase }>();
const rows = computed(() => timings(props.comparison, props.phase));
const category = computed(() => props.comparison === "10,000 marks" ? "Workload" : "Marks");
const phaseName = computed(() => props.phase === "first" ? "First render" : "Warm redraw");
const host = ref<HTMLDivElement>();
const error = ref("");

async function draw() {
    if (!host.value) return;
    try {
        await barChart(host.value, {
            mode: "grouped", width: 650, height: 305, yLabel: "Milliseconds",
            yTickMode: "integer", data: rows.value,
        });
        error.value = "";
    } catch (cause) {
        error.value = cause instanceof Error ? cause.message : String(cause);
    }
}
onMounted(draw);
watch(rows, draw);
</script>

<template>
    <figure class="chart-example benchmark-chart" :aria-label="phaseName + ' median times'">
        <div ref="host" class="chart-host"></div>
        <p v-if="error" role="alert">Couldn’t load the chart. {{ error }}</p>
    </figure>
    <table class="benchmark-table" :aria-label="phaseName + ' in milliseconds'">
        <thead><tr><th scope="col">{{ category }}</th><th v-for="r in renderers" :key="r.id" scope="col">{{ r.label }}</th></tr></thead>
        <tbody>
            <tr v-for="row in rows" :key="row.label">
                <th scope="row">{{ row.label }}</th>
                <td v-for="s in row.segments" :key="s.label">{{ s.value.toFixed(2) }}</td>
            </tr>
        </tbody>
    </table>
</template>
