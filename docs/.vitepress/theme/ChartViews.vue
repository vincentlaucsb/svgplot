<script setup lang="ts" generic="View extends string">
import { onMounted, ref, shallowRef } from "vue";

const props = defineProps<{
    label: string;
    views: readonly View[];
    draw: (host: HTMLElement, view: View) => Promise<SVGSVGElement>;
}>();
const host = ref<HTMLDivElement>();
const selected = shallowRef(props.views[0]);
const busy = ref(true);
const error = ref("");

async function show(view: View) {
    busy.value = true;
    error.value = "";
    try {
        await props.draw(host.value!, view);
        selected.value = view;
    } catch (cause) {
        error.value = cause instanceof Error ? cause.message : String(cause);
    } finally {
        busy.value = false;
    }
}

onMounted(() => show(selected.value));
</script>

<template>
    <figure class="chart-example">
        <div ref="host" class="chart-host" :aria-busy="busy"></div>
        <p v-if="error" role="alert">Couldn’t load the chart. {{ error }}</p>
        <figcaption>
            <span>{{ label }}</span>
            <div class="chart-views" role="group" :aria-label="label">
                <button v-for="view in views" :key="view" type="button"
                    :aria-pressed="selected === view" :disabled="busy" @click="show(view)">
                    {{ view }}
                </button>
            </div>
        </figcaption>
    </figure>
</template>
