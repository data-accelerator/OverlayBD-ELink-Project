# Streamlined Efficiency: Unshackling Kubernetes Image Volumes for Rapid AI Model and Dataset Loading

## Part 1. Background and the questions raised (Esteban Rey)

### Challenges in Managing Massive Data

When relying on datasets in Kubernetes there are a number of challenges that any data store must be able to account for:

- Data must be highly available
- Data access must be performant
- Data access must be scalable

In addition to these, there are a number of challenges users will face when trying to manage such data:

- Datasets often require versioning
- Data can grow exponentially and determining what to cleanup can be daunting.

### How OCI Registry implements dataset versioning and GC

OCI registries provide a multitude of solutions to each of the challenges managing massive datasets for Kubernetes requires, however, this has historically been primarily for container images which encompass only the runtime application users will run in Kubernetes. Nonetheless we can use registries to store datasets as container images, something that has been leveraged extensively before by users and even been formalized by the ORAS project.

Leveraging an oci/distribution registry as generic store solves a lot of the challenges, we have brough up before but introduces a few:

  - OCI Registries are highly available and must already scale to match user workloads as they already serve the images users need to run their infra.
  - OCI Registries provide versioning through tags and digests which provide data consistency in addition

While not a specified in the OCI spec, garbage collection is a common feature of most registries and the registry structure itself can be leveraged to clean unused container images.

### Disadvantages and Limitations to overcome with packaging data to OCI artifacts

Despite their many advantages, there are some considerations to storing datasets in image artifacts on registries:

  - Registries themselves are not data stores that were traditionally built for mounting their datasets in volumes, as a result, they may not perform well for volume mounting of large datasets without a streaming solution on top.
  - Registries have wells structured images built out of layers of an overlay filesystem, where each layer is typically a packaged tar file. This means any change to a dataset requires either repackaging or building on top of the existing system. Either way is expensive: 
  - Repackaging, which is the usual approach will require reindexing and recompressing the dataset and pushing it to a registry. This will also use lots of storage as registries cannot deduplicate the layers.
  - Building on top of previous layers requires downloading the existing layers and applying the overlay change as a new layer which still requires the previous layers to be extracted and eventually leads images with many layers. As images of datasets are typically large this can take some time

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