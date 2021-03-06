/*
 * Copyright © 2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2013 Inria.  All rights reserved.
 *
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#ifndef _NETLOC_MAP_H_
#define _NETLOC_MAP_H_

#include <netloc/autogen/config.h>
#include <netloc/rename_map.h>

#include <netloc.h>
#include <hwloc.h>

#ifdef __cplusplus
extern "C" {
#elif 0
}
#endif

/** \defgroup netloc_map_api Netloc Map API
 * @{
 */

typedef void * netloc_map_t;
typedef void * netloc_map_server_t;
typedef void * netloc_map_port_t;

/* creating a map */
NETLOC_DECLSPEC int netloc_map_create(netloc_map_t *map);
NETLOC_DECLSPEC int netloc_map_load_hwloc_data(netloc_map_t map, const char *data_dir);
NETLOC_DECLSPEC int netloc_map_load_netloc_data(netloc_map_t map, const char *data_dir);
enum netloc_map_build_flags_e {
  NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC = (1<<0)
};
NETLOC_DECLSPEC int netloc_map_build(netloc_map_t map, unsigned long flags);
/* destroying a map (needed even if build() failed) */
NETLOC_DECLSPEC int netloc_map_destroy(netloc_map_t map);

/* Returns the number of ports that are close to the hwloc topology and object.
 * On input, *nr specifies how many ports can be stored in *ports.
 * On output, *nr specifies how many were actually stored.
 *
 * If hobj is NULL, all ports of that server match.
 * If hobj is a I/O device, the matching ports that are returned are connected to that device.
 * Otherwise, the matching ports are connected to a I/O device close to hobj.
 */
NETLOC_DECLSPEC int netloc_map_hwloc2port(netloc_map_t map,
					  hwloc_topology_t htopo, hwloc_obj_t hobj,
					  netloc_map_port_t *ports, unsigned *nrp);

/* Given a netloc edge and or node in a netloc topology,
 * return the corresponding port.
 *
 * On input, one (and only one) of nedge and nnode may be NULL. If both are non-NULL, they should match.
 */
NETLOC_DECLSPEC int netloc_map_netloc2port(netloc_map_t map,
					   netloc_topology_t ntopo, netloc_node_t *nnode, netloc_edge_t *nedge,
					   netloc_map_port_t *port);
/* Return the netloc node+edge from a port.
 *
 * Some of nnode and nedgesmay be NULL if you don't care.
 */
NETLOC_DECLSPEC int netloc_map_port2netloc(netloc_map_port_t port,
					   netloc_topology_t *ntopo, netloc_node_t **nnode, netloc_edge_t **nedge);
/* Return the hwloc topology and object from a port.
 *
 * hobjp may be NULL if you don't care.
 *
 * htopop cannot be NULL. A reference will be taken on the topology, it should be released later with netloc_map_put_hwloc()
 */
NETLOC_DECLSPEC int netloc_map_port2hwloc(netloc_map_port_t port,
					  hwloc_topology_t *htopop, hwloc_obj_t *hobjp);

/* convert between a hwloc topology and a server.
 *
 * A reference is taken on the topology, it should be released later with netloc_map_put_hwloc()
 */
NETLOC_DECLSPEC int netloc_map_server2hwloc(netloc_map_server_t server, hwloc_topology_t *topology);
/* convert from topology to server.
 * equivalent to hwloc_obj_get_info_by_name(hwloc_get_root_obj(topology), "HostName") as long as hwloc stored the server name in the topology.
 * server should not be freed by the caller
 */
NETLOC_DECLSPEC int netloc_map_hwloc2server(netloc_map_t map, hwloc_topology_t topology, netloc_map_server_t *server);

/* release a hwloc topology pointer that we got above */
NETLOC_DECLSPEC int netloc_map_put_hwloc(netloc_map_t map, hwloc_topology_t topology);

/* FIXME: uniformize the following get() calls:
 * - get_subnets: the caller should free the array, not its contents (no internal array exists).
 * - get_servers: the caller should alloc/free the array, not its contents (no internal array exists).
 *   this one may be huge, but it's only useful for debugging.
 * - get_ports: the caller should not alloc/free anything (we return a copy of the internal array).
 */

/* get an array of subnets from the map.
 * the caller should free the array, not its contents.
 */
NETLOC_DECLSPEC int netloc_map_get_subnets(netloc_map_t map, unsigned *nr, netloc_topology_t **topos);

/* get the number of servers. */
NETLOC_DECLSPEC int netloc_map_get_nbservers(netloc_map_t map);

/* fill the input array with a range of servers.
 * servers must be allocated (and freed) by the caller.
 *
 * this function is not performance-optimized, it may be slow
 * when first is high.
 */
NETLOC_DECLSPEC int netloc_map_get_servers(netloc_map_t map,
					   unsigned first, unsigned nr,
					   netloc_map_server_t servers[]);

/* return the ports from the server.
 * the caller should not free the array.
 */
NETLOC_DECLSPEC int netloc_map_get_server_ports(netloc_map_server_t server,
						unsigned *nr, netloc_map_port_t **ports);

/* return the server from a port */
NETLOC_DECLSPEC int netloc_map_port2server(netloc_map_port_t port,
					   netloc_map_server_t *server);

/* return the map of a server */
NETLOC_DECLSPEC int netloc_map_server2port(netloc_map_server_t server, netloc_map_t *map);

/* return the name of a server */
NETLOC_DECLSPEC int netloc_map_server2name(netloc_map_server_t server, const char **name);

/* return the server from a name */
NETLOC_DECLSPEC int netloc_map_name2server(netloc_map_t map,
					   const char *name, netloc_map_server_t *server);



struct netloc_map_edge_s {
  enum netloc_map_edge_type_e {
    NETLOC_MAP_EDGE_TYPE_NETLOC,
    NETLOC_MAP_EDGE_TYPE_HWLOC_PARENT,
    NETLOC_MAP_EDGE_TYPE_HWLOC_HORIZONTAL,
    NETLOC_MAP_EDGE_TYPE_HWLOC_CHILD,
    NETLOC_MAP_EDGE_TYPE_HWLOC_PCI
  } type;
  union {
    struct {
      netloc_edge_t *edge;
      netloc_topology_t topology;
    } netloc;
    struct {
      hwloc_obj_t src_obj;
      hwloc_obj_t dest_obj;
      unsigned weight; /* 1 if there's a direct connection between them.
			* Otherwise the amount of edges between them on the shortest path across the hwloc tree.
			* Usually latency increases with this weigth while bandwidth decreases.
			* For PCI, the weight usually includes an edge from a NUMA node to a hostbridge,
			* some edges between PCI bridges/devices, and an edge from a PCI device to a OS device.
			*/
    } hwloc;
  };
};

/* By default only horizontal hwloc edges are reported, for instance cross-NUMA links */
enum netloc_map_paths_flag_e {
  NETLOC_MAP_PATHS_FLAG_IO = (1UL << 0), /* want edges between I/O objects such as PCI NICs and normal hwloc objects */
  NETLOC_MAP_PATHS_FLAG_VERTICAL = (1UL << 1) /* want edges between normal hwloc object child and parent, for instance from a core to a NUMA node */
};

typedef void * netloc_map_paths_t;

/* FIXME: add an optional subnet */
/* FIXME: make srcobj/dstobj optional. paths would only contain netloc edges. */
NETLOC_DECLSPEC int netloc_map_paths_build(netloc_map_t map,
					   hwloc_topology_t srctopo, hwloc_obj_t srcobj,
					   hwloc_topology_t dsttopo, hwloc_obj_t dstobj,
					   unsigned long flags,
					   netloc_map_paths_t *paths, unsigned *nr);

/* FIXME: also return a subnet pointer.
 * would be NULL if we ever have path across multiple subnets with gateways */
NETLOC_DECLSPEC int netloc_map_paths_get(netloc_map_paths_t paths, unsigned idx,
					 struct netloc_map_edge_s **edges, unsigned *nr_edges);

/* FIXME: get the distance (minimal path length, using PCI/NUMA/Eth/IB values */ 

NETLOC_DECLSPEC int netloc_map_paths_destroy(netloc_map_paths_t paths);


/* FIXME: get neighbor nodes at a given distance, within any or a single subnet */
/* FIXME: get neighbor nodes with enough cores, within any or a single subnet */

/* temporary, for debugging */
NETLOC_DECLSPEC int netloc_map_find_neighbors(netloc_map_t map,
					       const char *hostname, unsigned depth);

NETLOC_DECLSPEC int netloc_map_dump(netloc_map_t map);

#ifdef __cplusplus
}
#endif

/** @} */

/*
 * Ideas
 *
 * do users come with a predefined list of nodes (1), or are we going to tell them which nodes to use (2) ?
 *
 * If (1), one may want
 * * all distances/routes between all pairs of nodes within my set
 *   - with an algorithm that is faster than computing for each pair independently
 * * there could be a call computing everything and then a handle to retrieve the distance/route
 *   of a given pair?
 *
 * If (2), one  may want
 * * a set of nodes that are very close to each other
 *   - with or without a source node for this small neighborhood?
 *
 * For all of them, we may want variants with nodes only, and with cores
 * * give me 8 nodes or give enough nodes for 56 cores
 *
 * Distances can be expressed as
 * 3) a number of network hops, with optional link attributes
 * 4) a number of intra-node links
 *   On current architecture, only the number of inter-NUMA links matters.
 *   If the network ever goes to QPI without PCI, we may have to specify whether
 *   there are PCI bridges on the path. So keep the model flexible to add new types of links.
 *   If PCIe ever becomes a networks, we may to specify whether there is
 *
 * (4) may be negligible against (3), so it's not clear  whether we'll want a
 * full distance description (such as 1x NUMA + 1x PCI bridge + 2x network links.
 * (4) will be useful once you chose the nodes, and now need some cores on these,
 * which means we need a distance from cores to a subnet?
 */

#endif /* _NETLOC_MAP_H_ */
