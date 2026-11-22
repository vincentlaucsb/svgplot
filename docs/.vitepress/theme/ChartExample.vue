<script setup lang="ts">
import { onMounted, ref } from "vue";

const props = defineProps<{ interactive?: boolean }>();
const host = ref<HTMLDivElement>();
const button = ref<HTMLButtonElement>();
const ready = ref(false);
const error = ref("");

onMounted(async () => {
    const target = host.value!;
    const control = button.value;
    try {
        if (props.interactive && control) {
            const { mountReadings } = await import("../../examples/station-readings");
            await mountReadings(target, control);
        } else {
            const { drawFirstLine } = await import("../../examples/first-line");
            await drawFirstLine(target);
        }
        ready.value = true;
    } catch (cause) {
        error.value = cause instanceof Error ? cause.message : String(cause);
    }
});
</script>

<template>
    <figure class="chart-example">
        <div ref="host" class="chart-host" :aria-busy="!ready && !error"></div>
        <p v-if="error" role="alert">Couldn’t load the chart. {{ error }}</p>
        <figcaption v-if="interactive">
            <button ref="button" type="button" :disabled="!ready">Update readings</button>
            <span>Switch between the original and revised noon readings.</span>
        </figcaption>
    </figure>
</template>
