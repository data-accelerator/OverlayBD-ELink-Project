# Streamlined Efficiency: Unshackling Kubernetes Image Volumes for Rapid AI Model and Dataset Loading

## Part 1. Background and the questions raised (Esteban Rey)

### Challenges in Managing Massive Data

### How OCI Registry implements dataset versioning and GC

### Why is it so hard to packing data into an OCI artifacts

### ....

## Part 2. Solution (Yifan Yuan)

### Desribe a vision if a chaotic dataset could be managed through OCI, what problems could be solved?
   - Data versioned: Because the artifact's digest , users can realize the data changes before using it.
   - Garbage Collection: For an OCI artifact, by parsing the content of the manifest, one can identify the valid data within the entire bucket, thereby enabling garbage collection.

### The most difficult problem need to be solve
   - How to avoid to pack dataset into an OCI artifact
   - SOCI: a lazy loading container image solution without packing data // or TurboOCI ??

### ELink Solution
   - Introduce ReferenceList  
     expanding the index of gzip layer; make a possibility to index the hole storage bucket objects
   - How to organize the manifest of ReferenceList
   - POC based on overlaybd-turboOCI

### FAQ (Esrey & Yuan)