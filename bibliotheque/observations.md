# Laboratoire 4 — Observations

**Tests réalisés sur Raspberry Pi**

## Configuration 1 — SCHED_FIFO

Priorités : 93, 92, 91, 90  
Cœurs : 0, 0, 0, 0  
Durées (ms) : 5046, 5025, 5025, 5025

### Résultats de ps -L

```text
TID 1067 : SCHED_FIFO, priorité 93, cœur 0
TID 1068 : SCHED_FIFO, priorité 92, cœur 0
TID 1069 : SCHED_FIFO, priorité 91, cœur 0
TID 1070 : SCHED_FIFO, priorité 90, cœur 0
```

### Observation

J'ai remarqué que la tâche 1 terminait en premier, puis les autres suivaient dans l'ordre de leurs priorités. Comme elles utilisent toutes le même cœur, Linux laisse passer celle qui a la plus grande priorité en premier. Chaque tâche prend environ 5 secondes, et les calculs se font les uns après les autres.

---

## Configuration 2 — SCHED_FIFO

Priorités : 90, 90, 90, 90  
Cœurs : 0, 0, 0, 0  
Durées (ms) : 5078, 5024, 5025, 5024

### Résultats attendus de ps -L

```text
travail-1 : SCHED_FIFO, priorité 90, cœur 0
travail-2 : SCHED_FIFO, priorité 90, cœur 0
travail-3 : SCHED_FIFO, priorité 90, cœur 0
travail-4 : SCHED_FIFO, priorité 90, cœur 0
```

### Observation

Cette fois, les quatre tâches ont la même priorité et utilisent le même cœur. J'ai remarqué qu'elles terminaient encore une après l'autre, avec environ 5 secondes de calcul chacune. Même si elles ont la même priorité, FIFO permet à une tâche de garder le processeur jusqu'à ce qu'elle ait fini son travail.

---

## Configuration 2b — SCHED_FIFO

Priorités : 90, 90, 90, 90  
Cœurs : 0, 1, 2, 3  
Durées (ms) : 5572, 5569, 5571, 5571

### Résultats de ps -L

```text
TID 1189 : SCHED_FIFO, priorité 90, cœur 0
TID 1190 : SCHED_FIFO, priorité 90, cœur 1
TID 1191 : SCHED_FIFO, priorité 90, cœur 2
TID 1192 : SCHED_FIFO, priorité 90, cœur 3
```

### Observation

Ici, chaque tâche utilise un cœur différent. J'ai remarqué qu'elles commençaient et terminaient presque toutes en même temps, contrairement aux deux premières configurations. Elles prennent environ 5,5 secondes chacune, mais comme elles travaillent en parallèle, le programme termine les quatre calculs beaucoup plus rapidement au total.

---

## Configuration 3 — SCHED_RR

Priorités : 90, 90, 90, 90  
Cœurs : 0, 0, 0, 0  
Durées (ms) : 20025, 19994, 19923, 19847

### Résultats de ps -L

```text
TID 1246 : SCHED_RR, priorité 90, cœur 0
TID 1247 : SCHED_RR, priorité 90, cœur 0
TID 1248 : SCHED_RR, priorité 90, cœur 0
TID 1249 : SCHED_RR, priorité 90, cœur 0
```

### Observation

Dans cette configuration, j'ai remarqué que les quatre tâches prenaient environ 20 secondes pour terminer. Comme elles ont toutes la même priorité et utilisent le même cœur, Linux leur donne du temps de processeur chacune leur tour. Elles avancent donc en même temps, mais doivent se partager le cœur, ce qui augmente beaucoup leur durée.

---

## Configuration 4 — SCHED_RR

Priorités : 90, 90, 90, 91  
Cœurs : 0, 0, 0, 0  
Durées (ms) : 20051, 14952, 14878, 5085

### Résultats de ps -L

```text
TID 1304 : SCHED_RR, priorité 90, cœur 0
TID 1305 : SCHED_RR, priorité 90, cœur 0
TID 1306 : SCHED_RR, priorité 90, cœur 0
TID 1307 : SCHED_RR, priorité 91, cœur 0
```

### Observation

J'ai remarqué que la tâche 4 terminait beaucoup plus rapidement que les autres, en environ 5 secondes. C'est logique puisqu'elle a une priorité de 91, alors que les autres sont à 90. Linux lui donne donc la priorité, puis les trois autres doivent se partager le processeur avec Round Robin.

---

## Configuration 5 — SCHED_OTHER

Priorités : 0, 0, 0, 0  
Cœurs : libres (-1)  
Durées (ms) : 5070, 5074, 5070, 5072

### Résultats de ps -L

```text
TID 1364 : SCHED_OTHER, priorité 0, cœur 3
TID 1365 : SCHED_OTHER, priorité 0, cœur 0
TID 1366 : SCHED_OTHER, priorité 0, cœur 1
TID 1367 : SCHED_OTHER, priorité 0, cœur 0
```

### Observation

Pour ce dernier test, j'ai laissé Linux gérer les tâches normalement. Aucun cœur n'est imposé et les tâches n'ont pas de priorité temps réel. J'ai remarqué qu'elles terminaient presque toutes en même temps, après environ 5 secondes. Linux peut répartir les tâches sur plusieurs cœurs, ce qui leur permet de travailler en parallèle.

---

## Conclusion

Avec ces tests sur le Raspberry Pi, j'ai pu voir que les priorités et le nombre de cœurs influencent beaucoup l'exécution des tâches. J'ai surtout remarqué que SCHED_RR augmente les durées lorsque les tâches doivent partager un seul cœur, tandis qu'avec plusieurs cœurs, elles peuvent travailler en parallèle.

---

## Étape 5 — Race condition et checksum

*Tests réalisés précédemment dans la VM Debian.*

### Sans protection

Rafales écrites : 4 681 130  
Vérifications : 4 018 353  
Sommes fausses : 3 362 470

### Avec protection

Rafales écrites : 83 984 050  
Vérifications : 65 287 867  
Sommes fausses : 0

### Observation

Sans protection, j'ai remarqué qu'il y avait énormément de sommes fausses, parce que les threads pouvaient lire et modifier le tableau en même temps.

Quand j'ai activé le mutex avec la touche `c`, les sommes fausses sont restées à 0. C'est parce qu'un seul thread à la fois peut accéder au tableau, ce qui empêche les autres de modifier les valeurs pendant la vérification.